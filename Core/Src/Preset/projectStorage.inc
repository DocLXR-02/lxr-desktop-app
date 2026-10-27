/*
 * Bounded project replacement, private to presetManager.c.
 * SPDX-License-Identifier: GPL-3.0-only
 * See LICENSE and NOTICE.md. No filesystem work is performed from interrupts.
 */
#define PROJECT_STAGE "/LXRSTG"
#define PROJECT_BACKUP "/LXRBAK"
#define PROJECT_RECORD "/LXRTRANS.DAT"
#define PROJECT_FILES (2u + 3u * NUM_KIT_SLOTS)
#define PROJECT_PATTERN_BYTES (14u + NUM_TRACKS * NUM_STEPS * (sizeof(Step) + 1u) + NUM_TRACKS * NUM_BARS * 2u + 1u + NUM_TRACKS + 1u)
_Static_assert(NUM_TRACKS == 7 && NUM_KIT_SLOTS == 64, "project record/profile layout");

typedef struct {
    uint32_t oldRoot, oldHash, newRoot, newHash;
    uint16_t oldCount;
    uint8_t slot, ready;
} ProjectTransaction;

/* One foreground workspace; close DIR before opening FIL (_FS_LOCK == 1). */
static char preset_projectFiles[PROJECT_FILES][13];
_Alignas(4) static uint8_t preset_projectBytes[512];
static uint8_t preset_projectRecovered;

/* A failed mutating call may leave a dirty FatFs window. Do not flush/replay it
 * through unrelated cleanup. Logical precondition failures stay retryable. */
static FRESULT preset_txnMutation(FRESULT res)
{
    if(res == FR_DISK_ERR || res == FR_INT_ERR || res == FR_TIMEOUT ||
       res == FR_NOT_READY || res == FR_INVALID_OBJECT)
        if(preset_sharedFailure == FR_OK) preset_sharedFailure = res;
    return res;
}

static uint32_t preset_txnCrc(uint32_t crc, const void *data, UINT count)
{
    static const uint32_t table[16] = {
        0x00000000u,0x1DB71064u,0x3B6E20C8u,0x26D930ACu,
        0x76DC4190u,0x6B6B51F4u,0x4DB26158u,0x5005713Cu,
        0xEDB88320u,0xF00F9344u,0xD6D6A3E8u,0xCB61B38Cu,
        0x9B64C2B0u,0x86D3D2D4u,0xA00AE278u,0xBDBDF21Cu
    };
    const uint8_t *p = data;
    while(count--) {
        crc ^= *p++;
        crc = (crc >> 4) ^ table[crc & 15u];
        crc = (crc >> 4) ^ table[crc & 15u];
    }
    return crc;
}

static uint32_t preset_txnGet32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void preset_txnPut32(uint8_t *p, uint32_t value)
{
    for(uint8_t i = 0; i < 4; i++) p[i] = value >> (8u * i);
}

static uint8_t preset_txnEqual(const char *a, const char *b)
{
    for(;; a++, b++) {
        uint8_t x = *a, y = *b;
        if(x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if(y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if(x != y) return 0;
        if(!x) return 1;
    }
}

static int16_t preset_txnKey(const char *name)
{
    if(preset_txnEqual(name, "prj.nfo")) return 0;
    if(preset_txnEqual(name, "glo.cfg")) return 1;
    size_t n = strlen(name);
    if(n < 7 || n > 12) return -1;
    uint16_t slot;
    if(name[0] >= '0' && name[0] <= '9' && name[1] >= '0' && name[1] <= '9' && name[2] == '-')
        slot = (name[0] - '0') * 10u + name[1] - '0';
    else if(n == 8 && (name[0] == 'p' || name[0] == 'P')) {
        slot = 0;
        for(uint8_t i = 1; i < 4; i++) {
            if(name[i] < '0' || name[i] > '9') return -1;
            slot = slot * 10u + name[i] - '0';
        }
    } else return -1;
    if(slot >= NUM_KIT_SLOTS) return -1;
    const char *ext = name + n - 4;
    uint8_t type;
    if(preset_txnEqual(ext, ".snd")) type = 0;
    else if(preset_txnEqual(ext, ".pat")) type = 1;
    else if(preset_txnEqual(ext, ".sng")) type = 2;
    else return -1;
    return 2 + slot * 3 + type;
}

static FRESULT preset_txnDirectory(const char *folder, uint32_t *root)
{
    FILINFO info;
    *root = 0;
    FRESULT res = f_stat(folder, &info);
    if(res == FR_NO_FILE || res == FR_NO_PATH) return FR_OK;
    if(res != FR_OK) return res;
    if(!(info.fattrib & AM_DIR)) return FR_DENIED;
    DIR dir;
    res = f_opendir(&dir, folder);
    if(res != FR_OK) return res;
    *root = dir.obj.sclust;
    res = f_closedir(&dir);
    if(res != FR_OK) return preset_txnMutation(res);
    if(res == FR_OK && *root < 2) res = FR_INT_ERR;
    return res;
}

static FRESULT preset_txnInventory(const char *folder, uint32_t *root, uint16_t *count)
{
    DIR dir;
    FILINFO info;
    char path[21];
    *count = 0;
    memset(preset_projectFiles, 0, sizeof(preset_projectFiles));
    FRESULT res = f_opendir(&dir, folder);
    if(res != FR_OK) return res;
    *root = dir.obj.sclust;
    unsigned visited = 0;
    for(;;) {
        res = f_readdir(&dir, &info);
        if(res != FR_OK || !info.fname[0]) break;
        if(++visited > PROJECT_FILES + 2u) { res = FR_DENIED; break; }
        if(!strcmp(info.fname, ".") || !strcmp(info.fname, "..")) continue;
        if(info.fattrib & AM_DIR) { res = FR_DENIED; break; }
        res = preset_childPath(path, sizeof(path), folder, info.fname);
        if(res != FR_OK) break;
        int16_t key = preset_txnKey(info.fname);
        if(key < 0) { res = FR_DENIED; break; }
        if(preset_projectFiles[key][0]) { res = FR_EXIST; break; }
        memcpy(preset_projectFiles[key], info.fname, strlen(info.fname) + 1);
        (*count)++;
    }
    FRESULT closed = f_closedir(&dir);
    if(closed != FR_OK) preset_txnMutation(closed);
    return res == FR_OK ? closed : res;
}

static FRESULT preset_txnValidate(const char *folder, uint8_t strict,
                                  uint32_t *root, uint32_t *hash, uint16_t *count)
{
    FRESULT res = preset_txnInventory(folder, root, count);
    if(res != FR_OK) return res;
    if(!preset_projectFiles[0][0] || (strict && *count != PROJECT_FILES)) return FR_INT_ERR;
    uint32_t crc = 0xFFFFFFFFu;
    for(uint16_t key = 0; key < PROJECT_FILES; key++) {
        if(!preset_projectFiles[key][0]) continue;
        char path[21];
        res = preset_childPath(path, sizeof(path), folder, preset_projectFiles[key]);
        if(res != FR_OK) return res;
        res = preset_openShared((FIL*)&SDFile, path, FA_READ);
        if(res != FR_OK) return res;
        uint32_t size = f_size((FIL*)&SDFile);
        uint8_t type = key < 2 ? key : (key - 2) % 3 + 2;
        uint8_t prefix[14] = {0}, tail[8] = {0};
        if((type == 0 && size != 8) || (type == 1 && size != NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS) ||
           (type == 2 && (size < 8 || size > END_OF_SOUND_PARAMETERS + 8 || (strict && size != END_OF_SOUND_PARAMETERS + 8))) ||
           (type == 3 && (size < 8 || size > PROJECT_PATTERN_BYTES)) || (type == 4 && size != MAX_SONG_LENGTH + 8))
            res = FR_INT_ERR;
        uint8_t frame[6] = {key, key >> 8};
        preset_txnPut32(frame + 2, size);
        crc = preset_txnCrc(crc, frame, sizeof(frame));
        for(uint32_t offset = 0; res == FR_OK && offset < size;) {
            UINT got, request = size - offset;
            if(request > sizeof(preset_projectBytes)) request = sizeof(preset_projectBytes);
            res = f_read((FIL*)&SDFile, preset_projectBytes, request, &got);
            if(res == FR_OK && got != request) res = FR_INT_ERR;
            if(res != FR_OK) break;
            crc = preset_txnCrc(crc, preset_projectBytes, got);
            for(UINT i = 0; i < got; i++) {
                uint32_t at = offset + i;
                if(at < sizeof(prefix)) prefix[at] = preset_projectBytes[i];
                if(at >= size - 8) tail[at - (size - 8)] = preset_projectBytes[i];
            }
            offset += got;
        }
        if(res == FR_OK && type == 3) {
            uint8_t version = 1, header = 8;
            if(!memcmp(prefix + 8, "PatV", 4)) {
                if(prefix[12] != '0' || prefix[13] < '1' || prefix[13] > '3') res = FR_INT_ERR;
                else { version = prefix[13] - '0'; header = 14; }
            }
            uint32_t body = NUM_TRACKS * NUM_STEPS * (sizeof(Step) + (version > 2 ? 1u : 0u)) + NUM_TRACKS * NUM_BARS * 2u + 1u;
            uint32_t lengths = size >= header + body ? size - header - body : UINT32_MAX;
            if((strict && (version != 3 || header != 14)) ||
               (version == 1 ? (lengths != 0 && lengths != 7 && lengths != 8) : lengths != 8)) res = FR_INT_ERR;
            if(res == FR_OK && lengths >= 7) {
                for(uint8_t i = 0; i < NUM_TRACKS; i++)
                    if(tail[8 - lengths + i] > NUM_STEPS || (strict && !tail[8 - lengths + i])) res = FR_INT_ERR;
                if(version > 1 && tail[7] >= NUM_KIT_SLOTS) res = FR_INT_ERR;
            }
        }
        if(res == FR_OK && strict && key >= 2) {
            char expected[13];
            uint8_t ext = type == 2 ? FEXT_SOUND : type == 3 ? FEXT_PAT : FEXT_SONG;
            preset_makeFileNameSave(expected, (key - 2) / 3, ext, (char*)prefix);
            if(!preset_txnEqual(expected, preset_projectFiles[key])) res = FR_INT_ERR;
        }
        res = preset_finishRead(res);
        if(res != FR_OK) return res;
    }
    *hash = crc ^ 0xFFFFFFFFu;
    return FR_OK;
}

static void preset_txnHeader(uint8_t bytes[32], const ProjectTransaction *t)
{
    memset(bytes, 0, 32);
    memcpy(bytes, "LXRS4B2T", 8);
    bytes[8] = 1; bytes[9] = t->slot;
    bytes[10] = t->oldCount; bytes[11] = t->oldCount >> 8;
    preset_txnPut32(bytes + 12, t->oldRoot);
    preset_txnPut32(bytes + 16, t->oldHash);
    preset_txnPut32(bytes + 28, preset_txnCrc(0xFFFFFFFFu, bytes, 28) ^ 0xFFFFFFFFu);
}

static FRESULT preset_txnRead(ProjectTransaction *t)
{
    uint8_t bytes[48];
    UINT got;
    FRESULT res = preset_openShared((FIL*)&SDFile, PROJECT_RECORD, FA_READ);
    if(res != FR_OK) return res;
    uint32_t size = f_size((FIL*)&SDFile);
    res = f_read((FIL*)&SDFile, bytes, sizeof(bytes), &got);
    if(res == FR_OK && (size < 32 || size > 48 || got != size)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res != FR_OK) return res;
    if(memcmp(bytes, "LXRS4B2T", 8) || bytes[8] != 1 || bytes[9] >= NUM_KIT_SLOTS ||
       preset_txnGet32(bytes + 20) || preset_txnGet32(bytes + 24) ||
       preset_txnGet32(bytes + 28) != (preset_txnCrc(0xFFFFFFFFu, bytes, 28) ^ 0xFFFFFFFFu)) return FR_INT_ERR;
    memset(t, 0, sizeof(*t));
    t->slot = bytes[9];t->oldCount = bytes[10] | (uint16_t)bytes[11] << 8;
    t->oldRoot = preset_txnGet32(bytes + 12);t->oldHash = preset_txnGet32(bytes + 16);
    if(t->oldCount > PROJECT_FILES || (t->oldRoot ? t->oldRoot < 2 || !t->oldCount : t->oldCount || t->oldHash)) return FR_INT_ERR;
    if(size == 48 && bytes[40] == (uint8_t)PROJECT_FILES && bytes[41] == PROJECT_FILES >> 8 && !bytes[42] && !bytes[43] &&
       preset_txnGet32(bytes + 44) == (preset_txnCrc(0xFFFFFFFFu, bytes, 44) ^ 0xFFFFFFFFu)) {
        t->newRoot = preset_txnGet32(bytes + 32);t->newHash = preset_txnGet32(bytes + 36);
        if(t->newRoot < 2 || t->newRoot == t->oldRoot) return FR_INT_ERR;
        t->ready = 1;
    }
    return FR_OK; /* A partial/invalid footer never authorizes publication. */
}

static FRESULT preset_txnSeal(ProjectTransaction *t)
{
    uint8_t bytes[48];
    preset_txnHeader(bytes, t);
    preset_txnPut32(bytes + 32, t->newRoot);preset_txnPut32(bytes + 36, t->newHash);
    bytes[40] = PROJECT_FILES;bytes[41] = PROJECT_FILES >> 8;bytes[42] = bytes[43] = 0;
    preset_txnPut32(bytes + 44, preset_txnCrc(0xFFFFFFFFu, bytes, 44) ^ 0xFFFFFFFFu);
    FRESULT res = preset_openShared((FIL*)&SDFile, PROJECT_RECORD, FA_WRITE);
    if(res != FR_OK) return res;
    res = f_lseek((FIL*)&SDFile, 32);
    if(res == FR_OK) res = preset_writeChecked(bytes + 32, 16);
    res = preset_finishSave(res);
    if(res != FR_OK) return res;
    ProjectTransaction check;
    res = preset_txnRead(&check);
    if(res == FR_OK && (!check.ready || check.slot != t->slot || check.oldRoot != t->oldRoot ||
       check.newRoot != t->newRoot || check.newHash != t->newHash)) res = FR_INT_ERR;
    return res;
}

static FRESULT preset_txnMatches(const char *folder, uint8_t strict, uint32_t expectedRoot,
                                uint32_t expectedHash, uint16_t expectedCount)
{
    uint32_t root, hash;uint16_t count;
    FRESULT res = preset_txnValidate(folder, strict, &root, &hash, &count);
    if(res == FR_OK && (root != expectedRoot || hash != expectedHash || count != expectedCount)) res = FR_INT_ERR;
    return res;
}

static uint8_t preset_txnInvalid(FRESULT res)
{
    return res == FR_INT_ERR || res == FR_DENIED || res == FR_EXIST || res == FR_NO_FILE || res == FR_NO_PATH;
}

static FRESULT preset_txnChoose(const ProjectTransaction *t, const char *canonical,
                               uint32_t c, uint32_t s, uint32_t b, char chosen[8])
{
    FRESULT res;
    if(t->ready && c == t->newRoot) {
        res = preset_txnMatches(canonical, 1, c, t->newHash, PROJECT_FILES);
        if(res == FR_OK) { memcpy(chosen, canonical, 8); return res; }
        if(!preset_txnInvalid(res)) return res;
    }
    if(t->oldRoot && (c == t->oldRoot || b == t->oldRoot)) {
        const char *old = c == t->oldRoot ? canonical : PROJECT_BACKUP;
        res = preset_txnMatches(old, 0, t->oldRoot, t->oldHash, t->oldCount);
        if(res == FR_OK) { memcpy(chosen, old, 8); return res; }
        if(!preset_txnInvalid(res)) return res;
    }
    if(t->ready && s == t->newRoot) {
        res = preset_txnMatches(PROJECT_STAGE, 1, s, t->newHash, PROJECT_FILES);
        if(res == FR_OK) { memcpy(chosen, PROJECT_STAGE, 8); return res; }
        if(!preset_txnInvalid(res)) return res;
    }
    /* First-ever save: there is no older copy. A complete, owned but unsealed
     * stage is readable; incomplete files still fail the strict validator. */
    if(!t->ready && !t->oldRoot && !c && !b && s) {
        uint32_t root, hash;uint16_t count;
        res = preset_txnValidate(PROJECT_STAGE, 1, &root, &hash, &count);
        if(res == FR_OK) { memcpy(chosen, PROJECT_STAGE, 8); return res; }
        if(!preset_txnInvalid(res)) return res;
    }
    return FR_INT_ERR;
}

/* Delete only an owned, independently redundant, flat directory. */
static FRESULT preset_txnRemove(const char *folder, uint32_t expectedRoot)
{
    uint32_t root;uint16_t count;
    FRESULT res = preset_txnInventory(folder, &root, &count);
    if(res != FR_OK) return res;
    if(root != expectedRoot) return FR_INT_ERR;
    for(uint16_t key = 0; key < PROJECT_FILES; key++) {
        if(!preset_projectFiles[key][0]) continue;
        char path[21];
        res = preset_childPath(path, sizeof(path), folder, preset_projectFiles[key]);
        if(res != FR_OK) return res;
        res = preset_txnMutation(f_unlink(path));
        if(res != FR_OK) return res;
    }
    return preset_txnMutation(f_unlink(folder));
}

static FRESULT preset_txnIds(const char *canonical, uint32_t *c, uint32_t *s, uint32_t *b)
{
    FRESULT res = preset_txnDirectory(canonical, c);
    if(res == FR_OK) res = preset_txnDirectory(PROJECT_STAGE, s);
    if(res == FR_OK) res = preset_txnDirectory(PROJECT_BACKUP, b);
    return res;
}

static FRESULT preset_txnPrepare(uint8_t slot, const char *canonical)
{
    ProjectTransaction t;
    FRESULT res = preset_txnRead(&t);
    if(res == FR_NO_FILE) {
        uint32_t s, b;
        res = preset_txnDirectory(PROJECT_STAGE, &s);
        if(res == FR_OK) res = preset_txnDirectory(PROJECT_BACKUP, &b);
        return res != FR_OK ? res : s || b ? FR_EXIST : FR_OK;
    }
    if(res != FR_OK) return res;
    if(t.slot != slot) return FR_LOCKED;
    uint32_t c, s, b;
    res = preset_txnIds(canonical, &c, &s, &b);
    if(res != FR_OK) return res;
    /* Duplicate names can share one chain after an interrupted FAT rename. */
    if((c && (c == s || c == b)) || (s && s == b)) return FR_INT_ERR;
    char chosen[8];
    res = preset_txnChoose(&t, canonical, c, s, b, chosen);
    if(res != FR_OK) {
        /* No old project: discard only a demonstrably incomplete owned stage. */
        if(!t.oldRoot && !c && !b && preset_txnInvalid(res)) {
            uint32_t root, hash;uint16_t count;
            FRESULT valid = s ? preset_txnValidate(PROJECT_STAGE, 1, &root, &hash, &count) : FR_INT_ERR;
            if(valid == FR_OK || !preset_txnInvalid(valid)) return valid == FR_OK ? FR_INT_ERR : valid;
            if(s) { res = preset_txnRemove(PROJECT_STAGE, s); if(res != FR_OK) return res; }
            return preset_txnMutation(f_unlink(PROJECT_RECORD));
        }
        return res;
    }
    if(c && c != t.oldRoot && (!t.ready || c != t.newRoot)) return FR_EXIST;
    if(b && b != t.oldRoot) return FR_EXIST;
    if(s && t.ready && s != t.newRoot) return FR_EXIST;
    if(c && strcmp(chosen, canonical)) return FR_INT_ERR;
    if(!c && !b && !t.oldRoot && !t.ready && !strcmp(chosen, PROJECT_STAGE)) {
        /* Explicit user Save, after fresh validation: establish a new seal,
         * not a replay of project payload writes or automatic boot recovery. */
        uint16_t count;
        res = preset_txnValidate(PROJECT_STAGE, 1, &t.newRoot, &t.newHash, &count);
        if(res != FR_OK) return res;
        res = preset_txnSeal(&t);
        if(res != FR_OK) return res;
        t.ready = 1;
    }
    /* User-initiated Save only: restore/publish a validated reference first. */
    if(!c) {
        uint8_t restoring = !strcmp(chosen, PROJECT_BACKUP);
        const char *from = restoring ? PROJECT_BACKUP : PROJECT_STAGE;
        uint32_t expected = restoring ? b : s;
        res = preset_txnMutation(f_rename(from, canonical));
        if(res != FR_OK) return res;
        res = preset_txnIds(canonical, &c, &s, &b);
        if(res != FR_OK || c != expected || (restoring ? b != 0 : s != 0))
            return res == FR_OK ? preset_txnMutation(FR_INT_ERR) : res;
    }
    if(c == t.oldRoot) {
        if(b) return FR_EXIST;
        if(s) { res = preset_txnRemove(PROJECT_STAGE, s); if(res != FR_OK) return res; }
    } else if(t.ready && c == t.newRoot) {
        if(s) return FR_EXIST;
        if(b) { res = preset_txnRemove(PROJECT_BACKUP, b); if(res != FR_OK) return res; }
    } else return FR_INT_ERR;
    return preset_txnMutation(f_unlink(PROJECT_RECORD));
}

static FRESULT preset_txnSpace(void)
{
    DWORD available;
    FATFS *volume;
    FRESULT res = f_getfree("0:", &available, &volume);
    if(res != FR_OK) return res;
    if(!volume || !volume->csize) return FR_INT_ERR;
    uint32_t cluster = (uint32_t)volume->csize * _MAX_SS;
    uint32_t required = (8u + cluster - 1u) / cluster +
                        (NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS + cluster - 1u) / cluster;
    required += NUM_KIT_SLOTS * ((8u + END_OF_SOUND_PARAMETERS + cluster - 1u) / cluster +
                (PROJECT_PATTERN_BYTES + cluster - 1u) / cluster +
                (8u + MAX_SONG_LENGTH + cluster - 1u) / cluster);
    required += ((PROJECT_FILES + 2u) * 32u + cluster - 1u) / cluster;
    required += 2u; /* Transaction record and conservative root-directory growth. */
    return available >= required ? FR_OK : FR_DENIED;
}

static FRESULT preset_txnRun(uint8_t slot, const char *canonical)
{
    FRESULT res = preset_txnPrepare(slot, canonical);
    if(res != FR_OK) return res;
    ProjectTransaction t = {0};t.slot = slot;
    res = preset_txnDirectory(canonical, &t.oldRoot);
    if(res != FR_OK) return res;
    res = preset_txnSpace();
    if(res != FR_OK) return res;
    if(t.oldRoot) {
        res = preset_txnValidate(canonical, 0, &t.oldRoot, &t.oldHash, &t.oldCount);
        if(res != FR_OK) return res;
    }
    uint8_t header[32];preset_txnHeader(header, &t);
    res = preset_openShared((FIL*)&SDFile, PROJECT_RECORD, FA_CREATE_NEW | FA_WRITE);
    if(res != FR_OK) return res;
    res = preset_finishSave(preset_writeChecked(header, sizeof(header)));
    if(res != FR_OK) return res;
    ProjectTransaction check;
    res = preset_txnRead(&check);
    if(res != FR_OK || check.slot != slot || check.oldRoot != t.oldRoot || check.oldHash != t.oldHash || check.oldCount != t.oldCount)
        return res == FR_OK ? FR_INT_ERR : res;
    res = preset_txnMutation(preset_createProjectFolder(PROJECT_STAGE));
    if(res != FR_OK) return res;
    res = f_chdir(PROJECT_STAGE);
    if(res != FR_OK) return res;
    res = preset_writeProjectData(slot);
    if(res != FR_OK) return res;
    res = preset_txnValidate(PROJECT_STAGE, 1, &t.newRoot, &t.newHash, &check.oldCount);
    if(res != FR_OK) return res;
    res = preset_txnSeal(&t);
    if(res != FR_OK) return res;
    res = f_chdir("/"); /* Never rename/remove the current directory. */
    if(res != FR_OK) return res;
    if(t.oldRoot) {
        res = preset_txnMutation(f_rename(canonical, PROJECT_BACKUP));
        if(res != FR_OK) return res;
    }
    res = preset_txnMutation(f_rename(PROJECT_STAGE, canonical));
    if(res != FR_OK) return res;
    uint32_t root, s, b;
    res = preset_txnIds(canonical, &root, &s, &b);
    if(res != FR_OK) return res;
    if(root != t.newRoot || s || (t.oldRoot ? b != t.oldRoot : b != 0)) return preset_txnMutation(FR_INT_ERR);
    /* Same validated directory object, root-parent rename did not rewrite files. */
    char path[21];FILINFO info;
    res = preset_childPath(path, sizeof(path), canonical, "prj.nfo");
    if(res == FR_OK) res = f_stat(path, &info);
    if(res != FR_OK || info.fsize != 8 || (info.fattrib & AM_DIR)) return res == FR_OK ? preset_txnMutation(FR_INT_ERR) : res;
    if(b) { res = preset_txnRemove(PROJECT_BACKUP, b); if(res != FR_OK) return res; }
    res = preset_txnMutation(f_unlink(PROJECT_RECORD));
    if(res == FR_OK) { memcpy(currentFolder, canonical, 8);preset_projectRecovered = 0; }
    return res;
}

static FRESULT preset_txnLoadFolder(uint8_t slot, char folder[8])
{
    ProjectTransaction t;
    FRESULT res = preset_projectPath(folder, 8, slot);
    if(res != FR_OK) return res;
    preset_projectRecovered = 0;
    res = preset_txnRead(&t);
    if(res == FR_NO_FILE) return FR_OK;
    if(res != FR_OK) {
        /* Even failed recovery must not create files or overlay stale /tmp data. */
        preset_projectRecovered = 1;
        if(!preset_txnInvalid(res)) return res;
        uint32_t root, hash;uint16_t count;
        return preset_txnValidate(folder, 1, &root, &hash, &count);
    }
    if(t.slot != slot) return FR_OK;
    preset_projectRecovered = 1;
    uint32_t c, s, b;
    res = preset_txnIds(folder, &c, &s, &b);
    if(res != FR_OK) return res;
    char selected[8];
    res = preset_txnChoose(&t, folder, c, s, b, selected);
    if(res == FR_OK) { memcpy(folder, selected, 8);preset_projectRecovered = 1; }
    return res;
}

uint8_t preset_projectRecoveryActive(void)
{
    return preset_projectRecovered;
}

uint8_t preset_projectTransactionPending(void)
{
    FILINFO info;
    return f_stat(PROJECT_RECORD, &info) != FR_NO_FILE;
}
