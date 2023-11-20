#include <stdio.h>
#include <math.h>
#include <windows.h>

typedef struct ColorParams {
    // to get actual values: divide by 2^10 (precision equivalent to edid values)
    // on newer w11, the divisor is 2^20 instead
    unsigned int RedPoint[2];
    unsigned int GreenPoint[2];
    unsigned int BluePoint[2];
    unsigned int WhitePoint[2];
    // rest stored as nits * 10000
    unsigned int MinLuminance;
    unsigned int MaxLuminance;
    unsigned int MaxFullFrameLuminance;
} ColorParams;

typedef struct DISPLAYCONFIG_SET_ADVANCED_COLOR_PARAM {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    ColorParams colorParams;
    char stuff[4];
} DISPLAYCONFIG_SET_ADVANCED_COLOR_PARAM;

typedef struct DISPLAYCONFIG_GET_DISPLAY_INFO {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    char stuff1[1964];
    ColorParams colorParams;
    char stuff2[28];
} DISPLAYCONFIG_GET_DISPLAY_INFO;

enum DISPLAYCONFIG_DEVICE_INFO_TYPE_INTERNAL {
    _DISPLAYCONFIG_DEVICE_INFO_GET_DISPLAY_INFO = 0xFFFFFFFE,
    _DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_PARAM = 0xFFFFFFF0,
};

int main(int argc, char **argv) {
    int mode = -1;
    if (argc == 1) {
        mode = 0;
    } else if (argc == 3) {
        mode = 1;
    }
    if (mode == -1) {
        printf("Usage: set_maxtml [<monitor index> <nits>]\n");
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

            DISPLAYCONFIG_GET_DISPLAY_INFO displayInfo = {};
            displayInfo.header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE) _DISPLAYCONFIG_DEVICE_INFO_GET_DISPLAY_INFO;
            displayInfo.header.size = sizeof(displayInfo);
            displayInfo.header.adapterId = path.targetInfo.adapterId;
            displayInfo.header.id = path.sourceInfo.id;

            result = DisplayConfigGetDeviceInfo(&displayInfo.header);

            if (result != ERROR_SUCCESS) {
                fprintf(stderr, "Error on DisplayConfigGetDeviceInfo for display info\n");
                return 1;
            }

            int nits = (int) displayInfo.colorParams.MaxLuminance / 10000;

            printf("%d: %ls (%d nits)\n", i + 1, targetName.monitorFriendlyDeviceName, nits);
        }
    } else {
        int monitor = atoi(argv[1]);
        if (monitor <= 0 || monitor > pathCount) {
            fprintf(stderr, "Invalid monitor index %d\n", monitor);
            return 1;
        }
        int nits = atoi(argv[2]);
        if (nits <= 0 || nits > 10000) {
            fprintf(stderr, "Invalid nits value %d\n", nits);
        }

        DISPLAYCONFIG_PATH_INFO path = paths[monitor - 1];

        DISPLAYCONFIG_GET_DISPLAY_INFO displayInfo = {};
        displayInfo.header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE) _DISPLAYCONFIG_DEVICE_INFO_GET_DISPLAY_INFO;
        displayInfo.header.size = sizeof(displayInfo);
        displayInfo.header.adapterId = path.targetInfo.adapterId;
        displayInfo.header.id = path.sourceInfo.id;
        LONG result = DisplayConfigGetDeviceInfo(&displayInfo.header);

        if (result != ERROR_SUCCESS) {
            fprintf(stderr, "Error on DisplayConfigGetDeviceInfo for display info\n");
            return 1;
        }

        // keep old primary values
        ColorParams newParams = displayInfo.colorParams;
        // maxTML = maxFFTML for any sane display
        newParams.MaxLuminance = newParams.MaxFullFrameLuminance = nits * 10000;

        int divisor;
        // heuristic for getting the actual divisor used
        if (newParams.RedPoint[0] <= 1024) {
            divisor = 1 << 10;
        } else {
            divisor = 1 << 20;
        }

        // set some sane values while we're at it
        newParams.WhitePoint[0] = (int) roundf((0.3127f) * divisor);
        newParams.WhitePoint[1] = (int) roundf((0.3290f) * divisor);
        newParams.MinLuminance = 0;

        DISPLAYCONFIG_SET_ADVANCED_COLOR_PARAM advColorParams = {};
        advColorParams.header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE) _DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_PARAM;
        advColorParams.header.size = sizeof(advColorParams);
        advColorParams.header.adapterId = path.targetInfo.adapterId;
        advColorParams.header.id = path.targetInfo.id;

        advColorParams.colorParams = newParams;

        result = DisplayConfigSetDeviceInfo(&advColorParams.header);

        if (result != ERROR_SUCCESS) {
            fprintf(stderr, "Error on DisplayConfigSetDeviceInfo for advanced color params\n");
            return 1;
        }

        printf("Success\n");
    }
}
