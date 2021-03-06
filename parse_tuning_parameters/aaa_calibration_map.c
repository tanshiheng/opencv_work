#include "aaa_calibration_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define DEFAULT_FILE_PATH "mz_rcam_cali.bin.tmp"
int saveCalibrationMapToBin(const char *_file, const aaa_calibration_map *_map) {
    if (_file == NULL) {
        _file = DEFAULT_FILE_PATH;
    }

    FILE *fp = fopen(_file, "wb+");
    if (!fp) {
        printf("saveCalibrationMapToBin failed. open %s failed\n", _file);
        return -1;
    }
    int n = sizeof(aaa_calibration_map);
    if (fwrite(_map, n, 1, fp) != 1) {
        printf("saveCalibrationMapToBin failed. fwrite error: %d \n", ferror(fp));
        fclose(fp);
        return -1;
    }

    fclose(fp);

    printf("save %s success. file length: %lu \n", _file, sizeof(aaa_calibration_map));
    return 0;
}

int parseCalibrationMapFromBin(const char *_file, const aaa_calibration_map * _map) {
    if (_file == NULL) {
        _file = DEFAULT_FILE_PATH;
    }

    FILE *fp = fopen(_file, "rb");
    int n = 0;
    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    int requestSize = sizeof(aaa_calibration_map);

    if (fileSize < requestSize) {
        printf("bad file %s, fileSize = %d, request size = %d \n",
                _file, fileSize, requestSize);
        fclose(fp);
        return -1;
    }

    rewind(fp);

    if (fread((void*)_map, requestSize, 1, fp) != 1) {
        printf("parseCalibrationMapFromBin failed. fread error: %d \n", ferror(fp));
        fclose(fp);
        return -1;
    }

    printf ("parse %s success. file length: %d, request length: %d \n",
            _file, fileSize, requestSize);
    fclose(fp);
    return 0;
}

int parseCalibrationMapFromJson(const char * _file, aaa_calibration_map * _map) {
    struct stat status;
    if (stat(_file, &status) != 0) {
        printf("File %s not found!!!\n", _file);
        return -1;
    }
    int size = status.st_size;
    char *contents = (char *)malloc(size);

    FILE *fp = fopen(_file, "rb");
    if (fp == NULL) {
        printf("Unable to open %s \n", _file);
        free(contents);
        return -1;
    }

    if (fread(contents, size, 1, fp) != 1) {
        printf ("Unable to read %s \n" , _file);
        fclose(fp);
        free(contents);
        return -1;
    }

    fclose(fp);

    //printf ("%s\n", contents);

    json_value *value = json_parse((json_char *)contents, size);

    if (value == NULL) {
        printf("Unable to parse data");
        return -1;
    }

    // check the header of json value.
    {
        json_value * af = get_object("af", value);
        if (af && af->type == json_object) {
            parse_af_tuning_parameters(af, &_map->af);
        }

        json_value * ae = get_object("ae", value);
        if (ae && ae->type == json_object) {
            parse_ae_tuning_parameters(ae, &_map->ae);
        }

        json_value * gamma = get_object("gamma", value);
        if (gamma && gamma->type == json_object) {
            parse_gamma_tuning_parameters(gamma, &_map->gamma);
        }

        json_value * awb = get_object("awb", value);
        if (awb && awb->type == json_object) {
            parse_awb_tuning_parameters(awb, &_map->awb);
        }
    }
    json_value_free(value);
    free(contents);
    return 0;
}

int saveCalibrationMapToJson(const char * _file, const aaa_calibration_map * _map) {
    return 0;
}

void print_depth_shift(int depth) {
    int j;
    for (j=0; j < depth; j++) {
        printf(" ");
    }
}

void print_json_value(const json_value * value, int depth) {
    int j;
    if (value == NULL) {
        return;
    }
    if (value->type != json_object) {
        print_depth_shift(depth);
    }
    switch (value->type) {
        case json_none:
            printf("none\n");
            break;
        case json_object:
            print_object(value, depth+1);
            break;
        case json_array:
            print_array(value, depth+1);
            break;
        case json_integer:
            printf("int: %10" PRId64 "\n", value->u.integer);
            break;
        case json_double:
            printf("double: %f\n", value->u.dbl);
            break;
        case json_string:
            printf("string: %s\n", value->u.string.ptr);
            break;
        case json_boolean:
            printf("bool: %d\n", value->u.boolean);
            break;
    }
}

void print_object(const json_value *value,  int depth) {
    int length, x;
    if (value == NULL) {
        return;
    }
    length = value->u.object.length;
    for (x = 0; x < length; x++) {
        print_depth_shift(depth);
        printf("object[%d].name = %s\n", x, value->u.object.values[x].name);
        print_json_value(value->u.object.values[x].value, depth+1);
    }
}

void print_array(const json_value *value, int depth) {
    int length, x;
    if (value == NULL) {
        return;
    }
    length = value->u.array.length;
    printf("array\n");
    for (x = 0; x < length; x++) {
        print_json_value(value->u.array.values[x], depth);
    }
}

int parse_af_tuning_parameters(json_value *value, AF_TuningParameters *af) {
    int size, i;

    json_value *range = get_object("range", value);
    if (range) {
        af->rangeTune.infinityOffset = (int16_t)range->u.array.values[0]->u.integer;
        af->rangeTune.macroOffset = (int16_t)range->u.array.values[1]->u.integer;
        af->rangeTune.infinityFocusOffset = (int16_t)range->u.array.values[2]->u.integer;
        af->rangeTune.macroRangeStartOffset = (int16_t)range->u.array.values[3]->u.integer;
        af->rangeTune.macroRangeEndOffset = (int16_t)range->u.array.values[4]->u.integer;
        af->rangeTune.isManualRangeStartFromInfinity = (int8_t)range->u.array.values[5]->u.integer;
        af->rangeTune.reserved[0] = 0;
    }

    json_value *scene_classify = get_object("scene_classify", value);
    if (scene_classify) {
        af->sceneClassifyTune.lowContrastThreshold = (int64_t)scene_classify->u.array.values[0]->u.integer;
        af->sceneClassifyTune.highContrastThreshold = (int64_t)scene_classify->u.array.values[1]->u.integer;
        af->sceneClassifyTune.lowLightThreshold = (int64_t)scene_classify->u.array.values[2]->u.integer;
    }

    json_value *filter_weights = get_object("filter_weights", value);
    if (filter_weights && filter_weights->type == json_object) {
        json_value *weight_shift_big_filter = get_object("weight_shift_big_filter", filter_weights);
        if (weight_shift_big_filter && weight_shift_big_filter->type == json_array) {
            for (i = 0; i < 5; ++i) {
                af->filterWeightsTune.weightShiftBigFilter[i] =(int8_t) weight_shift_big_filter->u.array.values[i]->u.integer;
            }
        }

        json_value *weight_shift_row_filter = get_object("weight_shift_row_filter", filter_weights);
        if (weight_shift_row_filter && weight_shift_row_filter->type == json_array) {
            for (i = 0; i < 5; ++i) {
                af->filterWeightsTune.weightShiftRowFilter[i] = (int8_t) weight_shift_row_filter->u.array.values[i]->u.integer;
            }
        }

        json_value *weight_shift_gradient_filter = get_object("weight_shift_gradient_filter", filter_weights);
        if (weight_shift_gradient_filter && weight_shift_gradient_filter->type == json_array) {
            for (i = 0; i < 5; ++i) {
                af->filterWeightsTune.weightShiftGradientFilter[i] = (int8_t) weight_shift_gradient_filter->u.array.values[i]->u.integer;
            }
        }
    }

    json_value *step = get_object("step", value);
    if (step && step->type == json_array) {
        for (i = 0; i < AF_SEARCH_MODE_NUM; ++i) {
            af->moveStepTune.step[i] = (int16_t) step->u.array.values[i]->u.integer;
        }
    }

    json_value *window_size = get_object("window_size", value);
    if (window_size &&  window_size->type == json_array) {
        af->windowSizeTune.sizeOfTouch[0] = (int16_t) window_size->u.array.values[0]->u.integer;
        af->windowSizeTune.sizeOfTouch[1] = (int16_t) window_size->u.array.values[1]->u.integer;
        af->windowSizeTune.sizeOfContinuous[0] = (int16_t) window_size->u.array.values[2]->u.integer;
        af->windowSizeTune.sizeOfContinuous[1] = (int16_t) window_size->u.array.values[3]->u.integer;
    }

    json_value *delay = get_object("delay", value);
    if (delay && delay->type == json_array) {
        af->delayTune.startWarmingTime = (int32_t)delay->u.array.values[0]->u.integer;
        af->delayTune.isDelayOneFrameEachMove = (int8_t)delay->u.array.values[1]->u.integer;
        af->delayTune.firstMoveFrameDelay = (int8_t)delay->u.array.values[2]->u.integer;
        af->delayTune.infinityDelayOffset = (int16_t)delay->u.array.values[3]->u.integer;
        af->delayTune.dataTriggerFrameDelay = (int8_t)delay->u.array.values[4]->u.integer;
        af->delayTune.gyroTriggerFrameDelay = (int8_t)delay->u.array.values[5]->u.integer;
        af->delayTune.rangeTriggerFrameDelay = (int8_t)delay->u.array.values[6]->u.integer;
        af->delayTune.reserved[0] = 0;
    }

    json_value *scene_change = get_object("scene_change", value);
    if (scene_change && scene_change->type == json_array) {
        af->sceneChangeTune.dataTriggerRatioThreshold = (float) scene_change->u.array.values[0]->u.dbl;
        af->sceneChangeTune.gyroTriggerThreshold = (int) scene_change->u.array.values[1]->u.integer;
        af->sceneChangeTune.rangeTriggerThreshold = (int) scene_change->u.array.values[2]->u.integer;
        af->sceneChangeTune.dataStableRatioThreshold = (float) scene_change->u.array.values[3]->u.dbl;
        af->sceneChangeTune.gyroStableThreshold = (int) scene_change->u.array.values[4]->u.integer;
        af->sceneChangeTune.rangeStableThreshold = (int) scene_change->u.array.values[5]->u.integer;
        af->sceneChangeTune.isGyroTriggerEnabled = (int8_t) scene_change->u.array.values[6]->u.integer;
        af->sceneChangeTune.reserved[0] = 0;
        af->sceneChangeTune.reserved[1] = 0;
        af->sceneChangeTune.reserved[2] = 0;
    }
    json_value *search_pause = get_object("search_pause", value);
    if (search_pause && search_pause->type == json_array) {
        af->pauseTune.gyroPauseThreshold = (int) search_pause->u.array.values[0]->u.integer;
        af->pauseTune.rangePauseThreshold = (int) search_pause->u.array.values[1]->u.integer;
        af->pauseTune.gyroStableThreshold = (int) search_pause->u.array.values[2]->u.integer;
        af->pauseTune.rangeStableThreshold = (int) search_pause->u.array.values[3]->u.integer;
    }
    return 0;
}

int parse_ae_tuning_parameters(json_value *value, ae_tuning_parameters *ae) {
    int size, i;
    json_value *metering_config = get_object("metering_config", value);
    // metering_config
    if (metering_config && metering_config->type == json_object) {
        json_value *centerWidth = get_object("centerWidth", metering_config);
        if (centerWidth && centerWidth->type == json_integer) {
            ae->metering_config.centerWidth = (U32)centerWidth->u.integer;
        }

        json_value *centerHeight = get_object("centerHeight", metering_config);
        if (centerHeight && centerHeight->type == json_integer) {
            ae->metering_config.centerHeight = (U32)centerHeight->u.integer;
        }

        json_value *centerMap = get_object("centerMap", metering_config);
        if (centerMap && centerMap->type == json_array) {
            size = ae->metering_config.centerHeight * ae->metering_config.centerWidth;
            for (i = 0; i < size; ++i) {
                ae->metering_config.centerMap[i] = (U8) centerMap->u.array.values[i]->u.integer;
            }
        }

        json_value *spotWidth = get_object("spotWidth", metering_config);
        if (spotWidth && spotWidth->type == json_integer) {
            ae->metering_config.spotWidth = (U32)spotWidth->u.integer;
        }

        json_value *spotHeight = get_object("spotHeight", metering_config);
        if (spotHeight && spotHeight->type == json_integer) {
            ae->metering_config.spotHeight = (U32)spotHeight->u.integer;
        }

        json_value *spotMap = get_object("spotMap", metering_config);
        if (spotMap && spotMap->type == json_array) {
            size = ae->metering_config.spotHeight * ae->metering_config.spotWidth;
            for (i = 0; i < size; ++i) {
                ae->metering_config.spotMap[i] = (U8) spotMap->u.array.values[i]->u.integer;
            }
        }
    }

    //base config.
    json_value *base_config = get_object("base_config", value);
    if (base_config && base_config->type == json_object) {
        
        json_value *targetBrMin = get_object("targetBrMin", base_config);
        if (targetBrMin && targetBrMin->type == json_integer) {
            ae->base_config.targetBrMin = (U32) targetBrMin->u.integer;
        }

        json_value *targetBrMax = get_object("targetBrMax", base_config);
        if (targetBrMax && targetBrMax->type == json_integer) {
            ae->base_config.targetBrMax = (U32) targetBrMax->u.integer;
        }

        json_value *stableBrMin = get_object("stableBrMin", base_config);
        if (stableBrMin && stableBrMin->type == json_integer) {
            ae->base_config.stableBrMin = (U32) stableBrMin->u.integer;
        }

        json_value *stableBrMax = get_object("stableBrMax", base_config);
        if (stableBrMax && stableBrMax->type == json_integer) {
            ae->base_config.stableBrMax = (U32) stableBrMax->u.integer;
        }

        json_value *brLimitMin = get_object("brLimitMin", base_config);
        if (brLimitMin && brLimitMin->type == json_integer) {
            ae->base_config.brLimitMin = (U32) brLimitMin->u.integer;
        }

        json_value *brLimitMax = get_object("brLimitMax", base_config);
        if (brLimitMax && brLimitMax->type == json_integer) {
            ae->base_config.brLimitMax = (U32) brLimitMax->u.integer;
        }

        json_value *anaGainMax = get_object("anaGainMax", base_config);
        if (anaGainMax && anaGainMax->type == json_double) {
            ae->base_config.anaGainMax = (float) anaGainMax->u.dbl;
        }

        json_value *digGainMax = get_object("digGainMax", base_config);
        if (digGainMax && digGainMax->type == json_double) {
            ae->base_config.digGainMax = (float) digGainMax->u.dbl;
        }

        json_value *aperture = get_object("aperture", base_config);
        if (aperture && aperture->type == json_integer) {
            ae->base_config.aperture = (U32) aperture->u.integer;
        }

        json_value *expTimeDefault = get_object("expTimeDefault", base_config);
        if (expTimeDefault && expTimeDefault->type == json_integer) {
            ae->base_config.expTimeDefault = (U32) expTimeDefault->u.integer;
        }

        json_value *gainDefault = get_object("gainDefault", base_config);
        if (gainDefault && gainDefault->type == json_double) {
            ae->base_config.gainDefault = (float) gainDefault->u.dbl;
        }

        json_value *expTimeMin = get_object("expTimeMin", base_config);
        if (expTimeMin && expTimeMin->type == json_integer) {
            ae->base_config.expTimeMin = (U32) expTimeMin->u.integer;
        }
    }

    // convergent_table.
    json_value *convergent_table = get_object("convergent_table", value);
    if (convergent_table && convergent_table->type == json_object) {
        json_value *targetBr = get_object("targetBr", convergent_table);
        if (targetBr && targetBr->type == json_integer) {
            ae->convergent_table.targetBr = (U32) targetBr->u.integer;
        }

        json_value *tableCount1 = get_object("tableCount1", convergent_table);
        if (tableCount1 && tableCount1->type == json_integer) {
            ae->convergent_table.tableCount1 = (U32) tableCount1->u.integer;
        }

        json_value *table1 = get_object("table1", convergent_table);
        if (table1 && table1->type == json_array) {
            size = ae->convergent_table.tableCount1;
            for (i = 0; i < size; i++) {
                ae->convergent_table.table1[i].luminance = (U32) table1->u.array.values[2*i]->u.integer;
                ae->convergent_table.table1[i].convergent_ratio = (float) table1->u.array.values[2*i+1]->u.dbl;
            }
        }

        json_value *tableCount2 = get_object("tableCount2", convergent_table);
        if (tableCount2 && tableCount2->type == json_integer) {
            ae->convergent_table.tableCount2 = (U32) tableCount2->u.integer;
        }
        
        json_value *table2 = get_object("table2", convergent_table);
        if (table2 && table2->type == json_array) {
            size = ae->convergent_table.tableCount2;
            for (i = 0; i < size; i++) {
                ae->convergent_table.table2[i].luminance = (U32) table2->u.array.values[2*i]->u.integer;
                ae->convergent_table.table2[i].convergent_ratio = (float) table2->u.array.values[2*i+1]->u.dbl;
            }
        }

        json_value *flickerAutoCount = get_object("flickerAutoCount", convergent_table);
        if (flickerAutoCount && flickerAutoCount->type == json_integer) {
            ae->convergent_table.flickerAutoCount1 = (U32) flickerAutoCount->u.integer;
        }

        json_value *flickerAuto = get_object("flickerAuto", convergent_table);
        if (flickerAuto && flickerAuto->type == json_array) {
            size = ae->convergent_table.flickerAutoCount1 ;
            for (i = 0; i < size; i++) {
                ae->convergent_table.flickerAuto1[i] = (float) flickerAuto->u.array.values[i]->u.dbl;
            }
        }

        json_value *flicker50HzCount = get_object("flicker50HzCount", convergent_table);
        if (flicker50HzCount && flicker50HzCount->type == json_integer) {
            ae->convergent_table.flicker50HzTableCount1 = (U32) flicker50HzCount->u.integer;
        }

        json_value *flicker50Hz = get_object("flicker50Hz", convergent_table);
        if (flicker50Hz && flicker50Hz->type == json_array) {
            size = ae->convergent_table.flicker50HzTableCount1 ;
            for (i = 0; i < size; i++) {
                ae->convergent_table.flicker50Hz1[i] = (float) flicker50Hz->u.array.values[i]->u.dbl;
            }
        }


        json_value *flicker60HzCount = get_object("flicker60HzCount", convergent_table);
        if (flicker60HzCount && flicker60HzCount->type == json_integer) {
            ae->convergent_table.flicker60HzTableCount1 = (U32) flicker60HzCount->u.integer;
        }

        json_value *flicker60Hz = get_object("flicker60Hz", convergent_table);
        if (flicker60Hz && flicker60Hz->type == json_array) {
            size = ae->convergent_table.flicker60HzTableCount1 ;
            for (i = 0; i < size; i++) {
                ae->convergent_table.flicker60Hz1[i] = (float) flicker60Hz->u.array.values[i]->u.dbl;
            }
        }
    }

    //environment info.
    json_value *environment_info = get_object("environment_info", value);
    if (environment_info && environment_info->type == json_object) {

        if (environment_info && environment_info->type == json_object) {
            json_value *indoorBvRange = get_object("indoorBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(indoorBvRange, &ae->environment_info.indoorBvRange);

            json_value *normalBvRange = get_object("normalBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(normalBvRange, &ae->environment_info.normalBvRange);

            json_value *outdoorBvRange = get_object("outdoorBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(outdoorBvRange, &ae->environment_info.outdoorBvRange);

            json_value *underExposureBvRange = get_object("underExposureBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(underExposureBvRange, &ae->environment_info.underExposureBvRange);

            json_value *overExposureBvRange = get_object("overExposureBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(overExposureBvRange, &ae->environment_info.overExposureBvRange);

            json_value *totalBvRange = get_object("totalBvRange", environment_info);
            parse_ae_tuning_parameters_bv_range(totalBvRange, &ae->environment_info.totalBvRange);
        }
    }

    // dynamic_compensation
    json_value *dynamic_compensation = get_object("dynamic_compensation", value);
    if (dynamic_compensation && dynamic_compensation->type == json_object) {
        json_value * dynamic_ae_enabled = get_object("dynamic_ae_enabled", dynamic_compensation);
        if (dynamic_ae_enabled && dynamic_ae_enabled->type == json_integer) {
            ae->dynamic_compensation.dynamic_ae_enabled = (int) dynamic_ae_enabled->u.integer;
        }

        json_value * pureWhiteThreshold = get_object("pureWhiteThreshold", dynamic_compensation);
        if (pureWhiteThreshold && pureWhiteThreshold->type == json_double) {
            ae->dynamic_compensation.pureWhiteThreshold = (float) pureWhiteThreshold->u.dbl;
        }

        json_value * pureWhiteEnhanceMaxRatio = get_object("pureWhiteEnhanceMaxRatio", dynamic_compensation);
        if (pureWhiteEnhanceMaxRatio && pureWhiteEnhanceMaxRatio->type == json_double) {
            ae->dynamic_compensation.pureWhiteEnhanceMaxRatio = (float) pureWhiteEnhanceMaxRatio->u.dbl;
        }

        json_value * lowLightWeakRatio = get_object("lowLightWeakRatio", dynamic_compensation);
        if (lowLightWeakRatio && lowLightWeakRatio->type == json_double) {
            ae->dynamic_compensation.lowLightWeakRatio = (float) lowLightWeakRatio->u.dbl;
        }
    }
    return 0;
}

void parse_ae_tuning_parameters_bv_range(json_value *value, ae_bv_range * range) {
    if (value && value->type == json_array) {
        range->exposure1 = (float) value->u.array.values[0]->u.dbl;
        range->gain1 = (float) value->u.array.values[1]->u.dbl;
        range->y1 = (float) value->u.array.values[2]->u.dbl;

        range->exposure2 = (float) value->u.array.values[3]->u.dbl;
        range->gain2 = (float) value->u.array.values[4]->u.dbl;
        range->y2 = (float) value->u.array.values[5]->u.dbl;
    }
}

int parse_gamma_tuning_parameters(json_value *value, gamma_tuning_parameters *gamma) {
    int i, size;

    json_value *base_config = get_object("base_config", value);
    if (base_config && base_config->type == json_object) {
        json_value *normalGamma = get_object("normalGamma", base_config);
        if (normalGamma && normalGamma->type == json_double) {
            gamma->base_config.normalGamma = (float) normalGamma->u.dbl;
        }
        json_value *normalBaseOffset = get_object("normalBaseOffset", base_config);
        if (normalBaseOffset && normalBaseOffset->type == json_integer) {
            gamma->base_config.normalBaseOffset = (int) normalBaseOffset->u.integer;
        }
        json_value *normalEndOffset = get_object("normalEndOffset", base_config);
        if (normalEndOffset && normalEndOffset->type == json_integer) {
            gamma->base_config.normalEndOffset = (int) normalEndOffset->u.integer;
        }
        json_value *normalLinearityWeight = get_object("normalLinearityWeight", base_config);
        if (normalLinearityWeight && normalLinearityWeight->type == json_integer) {
            gamma->base_config.normalLinearityWeight = (U32) normalLinearityWeight->u.integer;
        }

        json_value *indoorGamma = get_object("indoorGamma", base_config);
        if (indoorGamma && indoorGamma->type == json_double) {
            gamma->base_config.indoorGamma = (float) indoorGamma->u.dbl;
        }
        json_value *indoorBaseOffset = get_object("indoorBaseOffset", base_config);
        if (indoorBaseOffset && indoorBaseOffset->type == json_integer) {
            gamma->base_config.indoorBaseOffset = (int) indoorBaseOffset->u.integer;
        }
        json_value *indoorEndOffset = get_object("indoorEndOffset", base_config);
        if (indoorEndOffset && indoorEndOffset->type == json_double) {
            gamma->base_config.indoorEndOffset = (int) indoorEndOffset->u.integer;
        }
        json_value *indoorLinearityWeight = get_object("indoorLinearityWeight", base_config);
        if (indoorLinearityWeight && indoorLinearityWeight->type == json_integer) {
            gamma->base_config.indoorLinearityWeight = (U32) indoorLinearityWeight->u.integer;
        }

        json_value *outdoorGamma = get_object("outdoorGamma", base_config);
        if (outdoorGamma && outdoorGamma->type == json_double) {
            gamma->base_config.outdoorGamma = (float) outdoorGamma->u.dbl;
        }
        json_value *outdoorBaseOffset = get_object("outdoorBaseOffset", base_config);
        if (outdoorBaseOffset && outdoorBaseOffset->type == json_integer) {
            gamma->base_config.outdoorBaseOffset = (int) outdoorBaseOffset->u.integer;
        }
        json_value *outdoorEndOffset = get_object("outdoorEndOffset", base_config);
        if (outdoorEndOffset && outdoorEndOffset->type == json_integer) {
            gamma->base_config.outdoorEndOffset = (int) outdoorEndOffset->u.integer;
        }
        json_value *outdoorLinearityWeight = get_object("outdoorLinearityWeight", base_config);
        if (outdoorLinearityWeight && outdoorLinearityWeight->type == json_integer) {
            gamma->base_config.outdoorLinearityWeight = (U32) outdoorLinearityWeight->u.integer;
        }

        json_value *gamma_table_x = get_object("gamma_table_x", base_config);
        if (gamma_table_x && gamma_table_x->type == json_array) {
            size = gamma_table_x->u.array.length;
            if (size != 32) {
                printf("base gamma table size: %d\n", size);
                size = (size > 32) ? 32 : size;
            }
            for (i = 0; i < size; ++i) {
                gamma->base_config.gamma_table_x[i] = (U32) gamma_table_x->u.array.values[i]->u.integer;
            }
        }

        json_value *user_gamma = get_object("user_gamma", base_config);
        if (user_gamma && user_gamma->type == json_array) {
            size = user_gamma->u.array.length;
            if (size != 32) {
                printf("base gamma table size: %d\n", size);
                size = (size > 32) ? 32 : size;
            }
            for (i = 0; i < size; ++i) {
                gamma->base_config.user_gamma[i] = (float) user_gamma->u.array.values[i]->u.dbl;
            }
        }
    }

    json_value *compensation = get_object("compensation", value);
    if (compensation && compensation->type == json_object) {
        json_value *drcEnabled = get_object("drcEnabled", compensation);
        if (drcEnabled && drcEnabled->type == json_integer) {
            gamma->compensation.drcEnabled = (int) drcEnabled->u.integer;
        }

        json_value *drcStrength = get_object("drcStrength", compensation);
        if (drcStrength && drcStrength->type == json_double) {
            gamma->compensation.drcStrength = (float) drcStrength->u.dbl;
        }
    }

    json_value *filter_table = get_object("filter_table", value);

    if (filter_table && filter_table->type == json_object) {
        // blackWhite
        json_value * blackWhite = get_object("blackWhite", filter_table);
        parse_gamma_tuning_parameters_filter(blackWhite, &gamma->filter_table.blackWhite);

        //negative
        json_value * negative = get_object("negative", filter_table);
        parse_gamma_tuning_parameters_filter(negative, &gamma->filter_table.negative);

        //bluish
        json_value * bluish = get_object("bluish", filter_table);
        parse_gamma_tuning_parameters_filter(bluish, &gamma->filter_table.bluish);
        //reserved
        json_value * reserved = get_object("reserved", filter_table);
        parse_gamma_tuning_parameters_filter(reserved, &gamma->filter_table.reserved);
        //reserved1
        json_value * reserved1 = get_object("reserved1", filter_table);
        parse_gamma_tuning_parameters_filter(reserved1, &gamma->filter_table.reserved1);
        //reserved2
        json_value * reserved2 = get_object("reserved2", filter_table);
        parse_gamma_tuning_parameters_filter(reserved2, &gamma->filter_table.reserved2);
        //reserved3
        json_value * reserved3 = get_object("reserved3", filter_table);
        parse_gamma_tuning_parameters_filter(reserved3, &gamma->filter_table.reserved3);
        //reserved4
        json_value * reserved4 = get_object("reserved4", filter_table);
        parse_gamma_tuning_parameters_filter(reserved4, &gamma->filter_table.reserved4);
    }
    return 0;
}

void parse_gamma_tuning_parameters_filter(json_value *filter, gamma_filter_item * item) {
    int i, j, size;
    if (filter && filter->type == json_object) {
        json_value *rgbFlag = get_object("rgbFlag", filter);
        if (rgbFlag && rgbFlag->type == json_array) {
            if (rgbFlag->u.array.length == RGB_CHANNEL)  {
                item->rgbFlag[0] = (U32) rgbFlag->u.array.values[0]->u.integer;
                item->rgbFlag[1] = (U32) rgbFlag->u.array.values[1]->u.integer;
                item->rgbFlag[2] = (U32) rgbFlag->u.array.values[2]->u.integer;
            } else {
                printf("bad rgbFlag array length (%d) for whiteBlack Filter.\n", rgbFlag->u.array.length);
            }
        }

        json_value *rgb = get_object("rgb", filter);
        if (rgb && rgb->type == json_array) {
            if (rgb->u.array.length == RGB_CHANNEL * GAMMA_TABLE_COUNT) {
                size = 0;
                for (i = 0; i < RGB_CHANNEL; ++i) {
                    for (j = 0; j < GAMMA_TABLE_COUNT; ++j, ++size) {
                        item->rgb[i][j] =
                            (U32)rgb->u.array.values[size]->u.integer;
                    }
                }
            } else {
                printf("bad rgb array length (%d) for whiteBlack Filter. \n", rgb->u.array.length);
            }
        }
    }
}

int parse_awb_tuning_parameters(json_value *value, awb_tuning_parameters *awb) {
    if (value && value->type == json_object) {
        json_value * white_point_gain = get_object("white_point_gain", value);
        if (white_point_gain && white_point_gain->type == json_array) {
            awb->white_point_gain.rGain = (int)white_point_gain->u.array.values[0]->u.integer;
            awb->white_point_gain.gGain = (int)white_point_gain->u.array.values[1]->u.integer;
            awb->white_point_gain.bGain = (int)white_point_gain->u.array.values[2]->u.integer;
        }

        json_value * white_point_ratio = get_object("white_point_ratio", value);
        if (white_point_ratio && white_point_ratio->type == json_array) {
            awb->white_point_ratio.rRatio = (float)white_point_ratio->u.array.values[0]->u.dbl;
            awb->white_point_ratio.gRatio = (float)white_point_ratio->u.array.values[1]->u.dbl;
            awb->white_point_ratio.bRatio = (float)white_point_ratio->u.array.values[2]->u.dbl;
        }

        json_value * manual_gain_mode = get_object("manual_gain_mode", value);
        if (manual_gain_mode && manual_gain_mode->type == json_integer) {
            awb->manual_gain_mode = (int)manual_gain_mode->u.integer;
        }

        json_value * manual_gain = get_object("manual_gain", value);
        if (manual_gain && manual_gain->type == json_array) {
            awb->manual_gain.rGain = (int)manual_gain->u.array.values[0]->u.integer;
            awb->manual_gain.gGain = (int)manual_gain->u.array.values[1]->u.integer;
            awb->manual_gain.bGain = (int)manual_gain->u.array.values[2]->u.integer;
        }

        json_value * macadam_ratio = get_object("macadam_ratio", value);
        if (macadam_ratio && macadam_ratio->type == json_double) {
            awb->macadam_ratio = (float)macadam_ratio->u.dbl;
        }

        json_value *color_sampling = get_object("color_sampling", value);
        if (color_sampling && color_sampling->type == json_array) {
            awb->color_sampling_high = (float)color_sampling->u.array.values[0]->u.dbl;
            awb->color_sampling_low = (float)color_sampling->u.array.values[1]->u.dbl;
        }

        json_value *surpressr = get_object("surpressr", value);
        if (surpressr && surpressr->type == json_double) {
            awb->surpressr = (float) surpressr->u.dbl;
        }

        json_value *saturation_default = get_object("saturation_default", value);
        if (saturation_default && saturation_default->type == json_integer) {
            awb->saturation_default = (int) saturation_default->u.integer;
        }

        json_value *delight_color = get_object("delight_color", value);
        if (delight_color && delight_color->type == json_integer) {
            awb->delight_color = (int) delight_color->u.integer;
        }

        json_value *chromatic_adaptation = get_object("chromatic_adaptation", value);
        if (chromatic_adaptation && chromatic_adaptation->type == json_double) {
            awb->chromatic_adaptation = (float) chromatic_adaptation->u.dbl;
        }

        json_value *g_norm_off_compensation = get_object("g_norm_off_compensation", value);
        if (g_norm_off_compensation && g_norm_off_compensation->type == json_integer) {
            awb->g_norm_off_compensation = (int) g_norm_off_compensation->u.integer;
        }


        json_value *scene = get_object("scene", value);
        if (scene && scene->type == json_object) {
            json_value *unknown = get_object("unknown", scene);
            parse_awb_tuning_parameters_scene(unknown, &awb->wb[WB_UNKNOWN_SCENE]);

            json_value *deepblue = get_object("deepblue", scene);
            parse_awb_tuning_parameters_scene(deepblue, &awb->wb[WB_DEEP_BLUE_SCENE]);

            json_value *d75 = get_object("d75", scene);
            parse_awb_tuning_parameters_scene(d75, &awb->wb[WB_D75_DAYLIGHT_SCENE]);

            json_value *d65 = get_object("d65", scene);
            parse_awb_tuning_parameters_scene(d65, &awb->wb[WB_D65_DAYLIGHT_SCENE]);

            json_value *d50 = get_object("d50", scene);
            parse_awb_tuning_parameters_scene(d50, &awb->wb[WB_D50_DAYLIGHT_SCENE]);

            json_value *yellowish = get_object("yellowish", scene);
            parse_awb_tuning_parameters_scene(yellowish, &awb->wb[WB_YELLOWISH_SCENE]);

            json_value *coldwhite = get_object("coldwhite", scene);
            parse_awb_tuning_parameters_scene(coldwhite, &awb->wb[WB_COLD_WHITE_SCENE]);

            json_value *tl84 = get_object("tl84", scene);
            parse_awb_tuning_parameters_scene(tl84, &awb->wb[WB_TL84_SCENE]);

            json_value *alight = get_object("alight", scene);
            parse_awb_tuning_parameters_scene(alight, &awb->wb[WB_ALIGHT_SCENE]);

            json_value *horizon = get_object("horizon", scene);
            parse_awb_tuning_parameters_scene(horizon, &awb->wb[WB_HORIZON_SCENE]);
        }
    }
    return 0;
}

void parse_awb_tuning_parameters_scene(json_value *scene, awb_white_balance *wb) {
    if (scene && scene->type == json_object) {
        json_value *gray_rb_ratio = get_object("gray_rb_ratio", scene);
        if (gray_rb_ratio && gray_rb_ratio->type == json_array) {
            wb->gray_rb_ratio.min = (float) gray_rb_ratio->u.array.values[0]->u.dbl;
            wb->gray_rb_ratio.max = (float) gray_rb_ratio->u.array.values[1]->u.dbl;
        }

        json_value *gray_fb_ratio = get_object("gray_fb_ratio", scene);
        if (gray_fb_ratio && gray_fb_ratio->type == json_array) {
            wb->gray_fb_ratio.min = (float) gray_fb_ratio->u.array.values[0]->u.dbl;
            wb->gray_fb_ratio.max = (float) gray_fb_ratio->u.array.values[1]->u.dbl;
        }

        json_value *wp_rb_ratio = get_object("wp_rb_ratio", scene);
        if (wp_rb_ratio && wp_rb_ratio->type == json_array) {
            wb->wp_rb_ratio.min = (float) wp_rb_ratio->u.array.values[0]->u.dbl;
            wb->wp_rb_ratio.max = (float) wp_rb_ratio->u.array.values[1]->u.dbl;
        }

        json_value *wp_fb_ratio = get_object("wp_fb_ratio", scene);
        if (wp_fb_ratio && wp_fb_ratio->type == json_array) {
            wb->wp_fb_ratio.min = (float) wp_fb_ratio->u.array.values[0]->u.dbl;
            wb->wp_fb_ratio.max = (float) wp_fb_ratio->u.array.values[1]->u.dbl;
        }

        json_value *scene_type = get_object("scene_type", scene);
        if (scene_type && scene_type->type == json_integer) {
            wb->scene_type = (int) scene_type->u.integer;
        }

        json_value *scene_name = get_object("scene_name", scene);
        if (scene_name && scene_name->type == json_string) {
            memcpy(wb->scene_name, scene_name->u.string.ptr, WB_SCENE_NAME_COUNT);
        }
    }
}

json_value *get_object(const char *key, json_value *value) {
    if (value->type != json_object) {
        printf("bad type for %s. \n", key);
        return NULL;
    }
    int size = value->u.object.length;
    int i;
    for (i = 0; i < size; ++i) {
        if (strcmp(key, value->u.object.values[i].name) == 0) {
            return value->u.object.values[i].value;
        }
    }
    return NULL;
}

int compareBinMap(const aaa_calibration_map * _map, const aaa_calibration_map *_map1) {
    int ret = 0;
    if (_map && _map1) {
        if (_map == _map1) {
            printf("same address \n");
            return ret;
        }

        if (memcmp(_map, _map1, sizeof (aaa_calibration_map)) == 0) {
            printf("the tow bins are same contents\n");
            return ret;
        }

        ret = -1;
        if (memcmp(&_map->af, &_map1->af, sizeof (AF_TuningParameters)) != 0) {
            printf("af segment are different!\n");
        }

        if (memcmp(&_map->ae, &_map1->ae, sizeof (ae_tuning_parameters)) != 0) {
            printf("ae segment are different!\n");
        }

        if (memcmp(&_map->gamma, &_map1->gamma, sizeof (gamma_tuning_parameters)) != 0) {
            printf("gamma segment are different!\n");
        }

        if (memcmp(&_map->awb, &_map1->gamma, sizeof (awb_tuning_parameters)) != 0) {
            printf("awb segment are different!\n");
        }
        return ret;
    }

    return -1;
}
