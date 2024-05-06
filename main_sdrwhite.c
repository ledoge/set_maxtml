#include <stdio.h>
#include <math.h>
#include <windows.h>

enum DISPLAYCONFIG_DEVICE_INFO_TYPE_INTERNAL {
    DISPLAYCONFIG_DEVICE_INFO_SET_SDR_WHITE_LEVEL = 0xFFFFFFEE,
};

typedef struct _DISPLAYCONFIG_SET_SDR_WHITE_LEVEL {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    unsigned int SDRWhiteLevel;
    unsigned char finalValue;
} _DISPLAYCONFIG_SET_SDR_WHITE_LEVEL;

LONG pathSetSdrWhite(DISPLAYCONFIG_PATH_INFO path, int nits) {
    _DISPLAYCONFIG_SET_SDR_WHITE_LEVEL sdrWhiteParams = {};
    sdrWhiteParams.header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE) DISPLAYCONFIG_DEVICE_INFO_SET_SDR_WHITE_LEVEL;
    sdrWhiteParams.header.size = sizeof(sdrWhiteParams);
    sdrWhiteParams.header.adapterId = path.targetInfo.adapterId;
    sdrWhiteParams.header.id = path.targetInfo.id;

    sdrWhiteParams.SDRWhiteLevel = nits * 1000 / 80;
    sdrWhiteParams.finalValue = 1;

    return DisplayConfigSetDeviceInfo(&sdrWhiteParams.header);
}

int main(int argc, char **argv) {
    int mode = -1;
    if (argc == 1) {
        mode = 0;
    } else if (argc == 3) {
        mode = 1;
    }
    if (mode == -1) {
        printf("Usage: set_sdrwhite [<monitor index> <nits>]\n");
        return 1;
    }

    DISPLAYCONFIG_PATH_INFO *paths = 0;
    DISPLAYCONFIG_MODE_INFO *modes = 0;
    UINT32 pathCount, modeCount;
    {
        UINT32 flags = QDC_ONLY_ACTIVE_PATHS;
        LONG result = ERROR_SUCCESS;

        do {
            if (paths) {
                free(paths);
            }
            if (modes) {
                free(modes);
            }

            result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);

            if (result != ERROR_SUCCESS) {
                fprintf(stderr, "Error on GetDisplayConfigBufferSizes\n");
                return 1;
            }

            paths = malloc(pathCount * sizeof(paths[0]));
            modes = malloc(modeCount * sizeof(modes[0]));

            result = QueryDisplayConfig(flags, &pathCount, paths, &modeCount, modes, 0);
            if (result != ERROR_SUCCESS && result != ERROR_INSUFFICIENT_BUFFER) {
                fprintf(stderr, "Error on QueryDisplayConfig\n");
                return 1;
            }
        } while (result == ERROR_INSUFFICIENT_BUFFER);
    }

    if (mode == 0) {
        for (int i = 0; i < pathCount; i++) {
            DISPLAYCONFIG_PATH_INFO path = paths[i];

            DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
            targetName.header.adapterId = path.targetInfo.adapterId;
            targetName.header.id = path.targetInfo.id;
            targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            targetName.header.size = sizeof(targetName);
            LONG result = DisplayConfigGetDeviceInfo(&targetName.header);

            if (result != ERROR_SUCCESS) {
                fprintf(stderr, "Error on DisplayConfigGetDeviceInfo for target name\n");
                return 1;
            }

            DISPLAYCONFIG_SDR_WHITE_LEVEL displayInfo = {};
            displayInfo.header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE) DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
            displayInfo.header.size = sizeof(displayInfo);
            displayInfo.header.adapterId = path.targetInfo.adapterId;
            displayInfo.header.id = path.targetInfo.id;

            result = DisplayConfigGetDeviceInfo(&displayInfo.header);

            if (result != ERROR_SUCCESS) {
                fprintf(stderr, "Error on DisplayConfigGetDeviceInfo for SDR white level\n");
                return 1;
            }

            int nits = (int) displayInfo.SDRWhiteLevel * 80 / 1000;

            printf("%d: %ls (%d nits)\n", i + 1, targetName.monitorFriendlyDeviceName, nits);
        }
    } else {
        int monitor = atoi(argv[1]);
        if (monitor < 0 || monitor > pathCount) {
            fprintf(stderr, "Invalid monitor index %d\n", monitor);
            return 1;
        }
        int nits = atoi(argv[2]);
        if (nits < 80 || nits > 480) {
            fprintf(stderr, "Invalid nits value %d\n", nits);
        }

        // round up to multiple of 4 to match sdr brightness slider increments
        if (nits % 4 != 0) {
            nits += 4 - (nits % 4);
        }

        if (monitor != 0) {
            DISPLAYCONFIG_PATH_INFO path = paths[monitor - 1];

            LONG result = pathSetSdrWhite(path, nits);

            if (result != ERROR_SUCCESS) {
                fprintf(stderr, "Error on DisplayConfigSetDeviceInfo for SDR white level\n");
                return 1;
            }

            printf("Success\n");

        } else {
            int success = 0;
            for (int i = 0; i < pathCount; i++) {
                DISPLAYCONFIG_PATH_INFO path = paths[i];

                LONG result = pathSetSdrWhite(path, nits);

                if (result != ERROR_SUCCESS) {
                    fprintf(stderr, "Error on DisplayConfigSetDeviceInfo for SDR white level\n");
                    continue;
                }
                success = 1;
            }

            if (!success) {
                return 1;
            }

            printf("Success\n");
        }
    }
}
