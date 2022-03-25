
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "maps_proto.h"
//-----------------------------------------------------------------------------

#define K_MAPS_PROTO_SOH 0x01
#define K_MAPS_PROTO_LF  0x0A
#define K_MAPS_PROTO_CR  0x0D

#define K_MAPS_PROTO_REQ_TYPE  0
#define K_MAPS_PROTO_RES_TYPE  1
#define K_MAPS_PROTO_UNK_TYPE  2
#define K_MAPS_PROTO_PASF_SIZE 88
#define K_MAPS_PROTO_SCSF_SIZE 12

#define param_error(e) do { errno = e; return NULL; } while (0)
#define parse_error(e) do { errno = e; goto PARSE_ERROR_EXEC; } while (0)
#define frame_error(e) do { errno = e; MapsProtoFreeRawFrame(frame); return NULL; } while(0)
//-----------------------------------------------------------------------------

///< @brief Function Pointer Callback for parse a request MAPS message.
typedef uint8_t (*RequestParseCb) (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
///< @brief Function Pointer Callback for parse a response MAPS message.
typedef uint8_t (*ResponseParseCb)(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);

/**
 *
 * @struct tMAPS_PROTO_CMD_INFO
 * @brief  Information of the MAPS command.
 *
 *         The supported barriers is indicated by the 3 first bits:
 *
 *         MSB (Third bit):  CF-220 barrier
 *         SSB (Second bit): CF-150
 *         LSB (First bit):  CF-24P
 *
 *         EXAMPLES:
 *
 *              111=All barriers are supported
 *              110=CF-220 & CF-150 barriers supported
 *              101=CF-220 & CF-24P barriers supported
 *              011=CF-150 & CF-24P barriers supported
 *
 *         The supported data refers to when the command accepts empty
 *         data in the request, the response or if it supports unknown response.
 *
 *         MSB (Third bit):  Unknown Responses.
 *         SSB (Second bit): Empty Request.
 *         LSB (First bit):  Empty Response.
 *
 *         EXAMPLES:
 *
 *         BR = 101 SUPPORT UNKNOWN RESPONSES    , NOT SUPPORT EMPTY REQUEST, SUPPORT EMPTY RESPONSE
 *         DE = 110 SUPPORT UNKNOWN RESPONSES    , SUPPORT EMPTY REQUEST    , NOT SUPPORT EMPTY RESPONSE
 *         MV = 111 SUPPORT UNKNOWN RESPONSES    , SUPPORT EMPTY REQUEST    , SUPPORT EMPTY RESPONSE
 *         RH = 100 SUPPORT UNKNOWN RESPONSES    , NOT SUPPORT EMPTY REQUEST, NOT SUPPORT EMPTY RESPONSE
 *         FR = 001 NOT SUPPORT UNKNOWN RESPONSES, NOT SUPPORT EMPTY REQUEST, SUPPORT EMPTY RESPONSE
 *         IR = 011 NOT SUPPORT UNKNOWN RESPONSES, SUPPORT EMPTY REQUEST    , SUPPORT EMPTY RESPONSE
 */
typedef struct
{
    uint8_t barriers;                    ///< Supported barriers.
    uint8_t suppdata;                    ///< The supported data.
    char cmd[K_MAPS_PROTO_CMD_LENGTH+1]; ///< The MAPS command. Is a NULL terminate string.
    RequestParseCb  RequestParseFunc;    ///< The request parse callback function.
    ResponseParseCb ResponseParseFunc;   ///< The response parse callback function.
}tMAPS_PROTO_CMD_INFO;
//-----------------------------------------------------------------------------

static const tMAPS_PROTO_CMD_INFO * MapsProtoFindCmd(const char *cmd);
static uint16_t  MapsProtoCalculateLRC     (uint8_t *data, uint16_t size);
static uint8_t   MapsProtoPrepareEMData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareEJData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareNoData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareAPData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareAJData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareTTData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareEAData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareDEData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareRHData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareSMData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareSCData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareCAData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareREData    (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareIARMData  (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareFailData  (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareDualData  (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareSingelData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareEndVehData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPreparePASpecial (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static uint8_t   MapsProtoPrepareSCSpecial (uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed);
static tMAPS_PROTO_RAW_FRAME * MapsProtoCreateFrame(uint8_t type, uint8_t num, const char *cmd, uint8_t *data, uint16_t data_size);
//-----------------------------------------------------------------------------

static tMAPS_PROTO_CMD_INFO cmd_data [] =
{
    { .barriers = 0b101, .suppdata = 0b101, .cmd = "BR" , .RequestParseFunc = MapsProtoPrepareSingelData, .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b100, .suppdata = 0b101, .cmd = "CA" , .RequestParseFunc = MapsProtoPrepareCAData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b110, .cmd = "DE" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareDEData    , },
    { .barriers = 0b101, .suppdata = 0b110, .cmd = "EA" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareEAData    , },
    { .barriers = 0b101, .suppdata = 0b100, .cmd = "ER" , .RequestParseFunc = MapsProtoPrepareDualData  , .ResponseParseFunc = MapsProtoPrepareSingelData, },
    { .barriers = 0b111, .suppdata = 0b111, .cmd = "FA" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b111, .cmd = "MV" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b111, .cmd = "PA" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b111, .cmd = "AC" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b100, .suppdata = 0b101, .cmd = "PR" , .RequestParseFunc = MapsProtoPrepareDualData  , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b111, .cmd = "RF" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b101, .suppdata = 0b101, .cmd = "SC" , .RequestParseFunc = MapsProtoPrepareSCData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b101, .cmd = "SM" , .RequestParseFunc = MapsProtoPrepareSMData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b100, .suppdata = 0b101, .cmd = "SR" , .RequestParseFunc = MapsProtoPrepareDualData  , .ResponseParseFunc = MapsProtoPrepareDualData  , },
    { .barriers = 0b111, .suppdata = 0b110, .cmd = "TT" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareTTData    , },
    { .barriers = 0b001, .suppdata = 0b100, .cmd = "RH" , .RequestParseFunc = MapsProtoPrepareRHData    , .ResponseParseFunc = MapsProtoPrepareRHData    , },
    { .barriers = 0b010, .suppdata = 0b110, .cmd = "CB" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareSingelData, },

    // THE NEXT 3 COMMANDS ARE SPECIAL SPONTANEOUS COMMANDS. INTERNAL USE ONLY
    { .barriers = 0b111, .suppdata = 0b001, .cmd = "PAS", .RequestParseFunc = MapsProtoPreparePASpecial , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b101, .suppdata = 0b001, .cmd = "SCS", .RequestParseFunc = MapsProtoPrepareSCSpecial , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b110, .suppdata = 0b001, .cmd = "FAS", .RequestParseFunc = MapsProtoPrepareEndVehData, .ResponseParseFunc = MapsProtoPrepareNoData    , },

    { .barriers = 0b111, .suppdata = 0b001, .cmd = "AJ" , .RequestParseFunc = MapsProtoPrepareAJData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b001, .cmd = "AP" , .RequestParseFunc = MapsProtoPrepareAPData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b100, .suppdata = 0b001, .cmd = "EJ" , .RequestParseFunc = MapsProtoPrepareEJData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b001, .cmd = "EM" , .RequestParseFunc = MapsProtoPrepareEMData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b001, .suppdata = 0b011, .cmd = "FP" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b110, .suppdata = 0b001, .cmd = "FR" , .RequestParseFunc = MapsProtoPrepareEndVehData, .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b001, .cmd = "FX" , .RequestParseFunc = MapsProtoPrepareFailData  , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b001, .suppdata = 0b011, .cmd = "IP" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b110, .suppdata = 0b011, .cmd = "IA" , .RequestParseFunc = MapsProtoPrepareIARMData  , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b110, .suppdata = 0b011, .cmd = "IR" , .RequestParseFunc = MapsProtoPrepareNoData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b001, .cmd = "PX" , .RequestParseFunc = MapsProtoPrepareFailData  , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b111, .suppdata = 0b011, .cmd = "RE" , .RequestParseFunc = MapsProtoPrepareREData    , .ResponseParseFunc = MapsProtoPrepareNoData    , },
    { .barriers = 0b110, .suppdata = 0b011, .cmd = "RM" , .RequestParseFunc = MapsProtoPrepareIARMData  , .ResponseParseFunc = MapsProtoPrepareNoData    , },
};
//-----------------------------------------------------------------------------

const tMAPS_PROTO_CMD_INFO * MapsProtoFindCmd(const char *cmd)
{
    if (cmd != NULL)
    {
        for (int i = 0, size = sizeof(cmd_data)/sizeof(cmd_data[0]); i < size; i++)
        {
             if (strcmp(cmd_data[i].cmd,cmd) == 0)
                 return &cmd_data[i];
        }
    }

    return NULL;
}
//-----------------------------------------------------------------------------

uint16_t MapsProtoCalculateLRC(uint8_t *data, uint16_t size)
{
    uint16_t lrc;
    uint8_t clrc[3];
    uint8_t xsum = 0;

    for (uint16_t i = 0; i < size; i++)
         xsum ^= data[i];

    sprintf((char *)clrc,"%.2X",xsum);
    clrc[0] = (clrc[0] > 0x39) ? (clrc[0] - 7) : clrc[0];
    clrc[1] = (clrc[1] > 0x39) ? (clrc[1] - 7) : clrc[1];
    memcpy(&lrc,clrc,2);

    return lrc;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareNoData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    (void) frame;
    uint16_t fsize = (parsed->type) ? 9 : 7;  // The size of the frame. When response must be 9 on request 7

    if (size == fsize)
        return 0;

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareEMData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_EM_DATA *data;

    if (size == 16 || size == 17)
    {
        if ((data = (tMAPS_PROTO_EM_DATA *)calloc(1,sizeof(tMAPS_PROTO_EM_DATA))) == NULL)
            return 1;

        parsed->size = sizeof(tMAPS_PROTO_EM_DATA);
        parsed->data = (char *) data;

        if (!isdigit(frame[4]) || (frame[4] - 48) > 3)               // Mode
            return 2;
        if (!isxdigit(frame[5]))                                     // Axes
            return 2;
        if (!isdigit(frame[6]) || (frame[6] - 48) > 2)               // Hights
            return 2;

        data->work_mode   = frame[4] - 48;
        data->axis_ispeed = (frame[5] < 58) ? (frame[5] - 48) : (frame[5] - 55);
        data->axis_height = frame[6] - 48;

        if (size == 16)                             // CF-150 & CF-24P
        {
            if (frame[7] != 49 && frame[7] != 50 && frame[7] != 51)  // HW Failure
                return 2;
            if (frame[8] != 49 && frame[8] != 50)                    // Cleaning
                return 2;
            if (!isdigit(frame[9]) || !isdigit(frame[10]))           // Firmware
                return 2;

            data->hw_failure   = frame[7] - 48;
            data->se_cleaning  = frame[8] - 48;
            data->firmware_ver = ((frame[9] - 48) * 10) + (frame[10] - 48);
        }
        else                                        // CF-220
        {
            if (frame[7] != 48 && frame[7] != 'R' && frame[7] != 'M' && frame[7] != 'N' && frame[7] != 'E' && frame[7] != 'T')  // Tow
                return 2;
            if (frame[8] != 49 && frame[8] != 50 && frame[8] != 51)  // HW Failure
                return 2;
            if (frame[9] != 49 && frame[9] != 50)                    // Cleaning
                return 2;
            if (!isdigit(frame[10]) || !isdigit(frame[11]))          // Firmware
                return 2;
            if (frame[12] != 'P' && frame[13] != 'N')                // Direction
                return 2;

            data->tow_detection  = frame[7];
            data->hw_failure     = frame[8] - 48;
            data->se_cleaning    = frame[9] - 48;
            data->firmware_ver   = ((frame[10] - 48) * 10) + (frame[11] - 48);
            data->rcvr_direction = frame[12];
        }

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareEJData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_EJ_DATA *data;

    if (size == 13)
    {
        for (uint8_t i = 4; i < 10; i++) {
             if (!isdigit(frame[i]))
                 return 2;
        }

        if ((data = (tMAPS_PROTO_EJ_DATA *)calloc(1,sizeof(tMAPS_PROTO_EJ_DATA))) == NULL)
            return 1;

        data->paxes  = ((frame[4] - 48) * 10) + (frame[5] - 48);
        data->naxes  = ((frame[6] - 48) * 10) + (frame[7] - 48);
        data->ispeed = ((frame[8] - 48) * 10) + (frame[9] - 48);

        parsed->size = sizeof(tMAPS_PROTO_EJ_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareAPData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_AP_DATA *data = (tMAPS_PROTO_AP_DATA *)calloc(1,sizeof(tMAPS_PROTO_AP_DATA));

    if (data == NULL)
        return 1;

    parsed->data = (char *) data;
    parsed->size = sizeof(tMAPS_PROTO_AP_DATA);

    if (size == 9)        // IS A CF-150 BARRIER OR THIRD SM BYTE IS 0 [CF-24P] OR IS 1 [CF-220]
    {
        if (!isdigit(frame[4]) || !isdigit(frame[5]))
            return 2;

        data->smbyte  = 0;
        data->vheight = ((frame[4] - 48) * 10) - (frame[5] - 48);
        return 0;
    }
    else if (size == 17)  // ONLY CF-220 & CF-24P WHEN THIRD SM BYTE IS 2.
    {
        if (frame[4] != 48 && frame[4] != 'N' && frame[4] != 'P')
            return 2;

        for (uint8_t i = 6; i < 14; i++) {
            if (!isdigit(frame[i]))
                return 2;
        }

        data->smbyte  = 2;
        data->vaxis = frame[4];

        if (data->axis_height > 15) // The max value accoding to MAPS documentation is 15
            return 2;

        data->axis_height = ((frame[6]   - 48) * 10) + (frame[7]  - 48);
        data->vmax_height = ((frame[8]   - 48) * 10) + (frame[9]  - 48);
        data->hmin_height = ((frame[10]  - 48) * 10) + (frame[11] - 48);
        data->lmax_height = ((frame[12]  - 48) * 10) + (frame[13] - 48);

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareAJData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_BARRIER_ADJUST *data;

    if (size == 95)
    {
        if ((data = (tMAPS_PROTO_BARRIER_ADJUST *)calloc(1,sizeof(tMAPS_PROTO_BARRIER_ADJUST))) == NULL)
            return 1;

        memcpy(data->rcv_map8,&frame[4],K_MAPS_PROTO_RECEIVE_GROUP8);
        memcpy(data->rcv_map3,&frame[4+K_MAPS_PROTO_RECEIVE_GROUP8],K_MAPS_PROTO_RECEIVE_GROUP3);
        parsed->size = sizeof(tMAPS_PROTO_BARRIER_ADJUST);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareTTData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_TT_DATA *data;

    if (size == 35)
    {
        if (frame[6] != 'M' || frame[23] != 'R')
            return 2;

        for (uint8_t i = 7; i <= 31; i++)
        {
            if (i != 23 && !isxdigit(frame[i]))
                return 2;
        }

        if ((data = (tMAPS_PROTO_TT_DATA *)calloc(1,sizeof(tMAPS_PROTO_TT_DATA))) == NULL)
            return 1;

        data->mvar = frame[6];
        data->rvar = frame[23];
        memcpy(data->e_map,&frame[7],K_MAPS_PROTO_EMITTERS_MAP_SIZE);
        memcpy(data->r_map,&frame[24],K_MAPS_PROTO_RECEIVERS_MAP_SIZE);
        parsed->size = sizeof(tMAPS_PROTO_TT_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareEAData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_EA_DATA *data;

    if (size == 17)
    {
        for (uint8_t i = 6; i <= 13; i++) {
             if (!isdigit(frame[i]))
                 return 2;
        }

        if ((data = (tMAPS_PROTO_EA_DATA *)calloc(1,sizeof(tMAPS_PROTO_EA_DATA))) == NULL)
            return 1;

        data->imax_height = ((frame[6]  - 48) * 10) + (frame[7]  - 48);
        data->umax_height = ((frame[8]  - 48) * 10) + (frame[9]  - 48);
        data->umin_height = ((frame[10] - 48) * 10) + (frame[11] - 48);
        data->lmax_height = ((frame[12] - 48) * 10) + (frame[13] - 48);
        parsed->size = sizeof(tMAPS_PROTO_EA_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareDEData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_DE_DATA *data;

    if (size == 19)
    {
        if (!isdigit(frame[6]) || (frame[6] - 48) > 3)   // Mode
            return 2;
        if (!isxdigit(frame[7]))                         // Axes
            return 2;
        if (!isdigit(frame[8]) || (frame[8] - 48) > 2)   // Hights
            return 2;
        if (frame[9] != 48 && frame[9] != 'R' && frame[9] != 'M' && frame[9] != 'N' && frame[9] != 'E' && frame[9] != 'T') // Tow
            return 2;
        if (!isdigit(frame[10]) && frame[10] != 49 && frame[10] != 50 && frame[10] != 51) // Sensors
            return 2;
        if (!isdigit(frame[11]) && frame[11] != 49 && frame[11] != 50)                    // Cleaning
            return 2;
        if (!isdigit(frame[12]) || !isdigit(frame[13]))  // Firmware
            return 2;
        if ((data = (tMAPS_PROTO_DE_DATA *)calloc(1,sizeof(tMAPS_PROTO_DE_DATA))) == NULL)
            return 1;

        data->work_mode      = frame[6] - 48;
        data->axis_ispeed    = (frame[7] < 58) ? (frame[7] - 48) : (frame[7] - 55);
        data->axis_height    = frame[8] - 48;
        data->tow_detection  = frame[9];
        data->hw_failure     = frame[10] - 48;
        data->se_cleaning    = frame[11] - 48;
        data->firmware_ver   = ((frame[12] - 48) * 10) + (frame[13] - 48);
        data->rcvr_direction = frame[14];
        data->barrier_model  = frame[15];
        parsed->size = sizeof(tMAPS_PROTO_DE_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareRHData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    uint8_t number;
    tMAPS_PROTO_RH_DATA *data;
    uint8_t   dpos = (parsed->type) ?  6 :  4; // The start data position. In response start at pos 6 on request at pos 4
    uint16_t fsize = (parsed->type) ? 12 : 10; // The size of the frame. When response must be 12 on request 10

    if (size == fsize)
    {
        number = ((frame[dpos+1] - 48) * 10) + (frame[dpos+2] - 48);

        if (frame[dpos] != 48 && frame[dpos] != 49)
            return 2;
        if (number < 1 || number > 24)
            return 2;
        if ((data = (tMAPS_PROTO_RH_DATA *)calloc(1,sizeof(tMAPS_PROTO_RH_DATA))) == NULL)
            return 1;

        data->wmode  = frame[dpos] - 48;
        data->recvn  = number;
        parsed->size = sizeof(tMAPS_PROTO_RH_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareSMData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_SM_DATA *data;

    if (size >= 10 && size <= 12)
    {
        if (frame[4] != 48 && frame[4] != 49 && frame[4] != 50 && frame[4] != 51)
            return 2;
        if (!isxdigit(frame[5]))
            return 2;
        if (frame[6] != 48 && frame[6] != 49 && frame[6] != 50)
            return 2;
        if (size >= 11 && frame[7] != 48 && frame[7] != 'R' && frame[7] != 'M' && frame[7] != 'N' && frame[7] != 'E' && frame[7] != 'T')
            return 2;
        if (size == 12 && frame[8] != 'P' && frame[8] != 'N')
            return 2;
        if ((data = (tMAPS_PROTO_SM_DATA *)calloc(1,sizeof(tMAPS_PROTO_SM_DATA))) == NULL)
            return 1;

        data->work_mode      = frame[4] - 48;
        data->axis_ispeed    = (frame[5] < 58) ? (frame[5] - 48) : (frame[5] - 55);
        data->axis_height    = frame[6] - 48;
        data->tow_detection  = (size >= 11) ? frame[7] : 48;
        data->rcvr_direction = (size == 12) ? frame[8] : 48;
        parsed->size = sizeof(tMAPS_PROTO_SM_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareSCData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    char stime[4] = {0};

    if (size == 11)      // SC REQUEST. HAVE 4 BYTES IN DATA
    {
        tMAPS_PROTO_SC_DATA *data;

        if (frame[4] != 'A' && frame[4] != 'B' && frame[4] != 'C' &&
            frame[4] != 'D' && frame[4] != 'E' && frame[4] != 'H' && frame[4] != 'I')
            return 2;
        if (!isdigit(frame[5]) || !isdigit(frame[6]) || !isdigit(frame[7]))
            return 2;
        if ((data = (tMAPS_PROTO_SC_DATA *)calloc(1,sizeof(tMAPS_PROTO_SC_DATA))) == NULL)
            return 1;

        memcpy(stime,&frame[5],3);
        data->mode      = frame[4];
        data->send_time = atoi(stime);
        parsed->size = sizeof (tMAPS_PROTO_SC_DATA);
        parsed->data = (char *) data;

        return 0;
    }
    else if (size == 15) // SC ESPECIAL WITH MAPS PROTOCOL STRUCTURE. ONLY WITH [CF-24P]. MODES A,B,C HAVE 8 BYTES IN DATA
    {
        tMAPS_PROTO_SC_SPECIAL *data;

        if ((frame[4] - 48) > 1)
            return 2;

        for (uint8_t i = 5; i < 11; i++) {
             if (!isxdigit(frame[i]))
                 return 2;
        }

        if (!isdigit(frame[11]))
            return 2;
        if ((data = (tMAPS_PROTO_SC_SPECIAL*)calloc(1,sizeof(tMAPS_PROTO_SC_SPECIAL))) == NULL)
            return 1;

        data->mode = 'A';
        data->MODES.ABCMODES.presence   = frame[4] - 48;
        data->MODES.ABCMODES.sweeps_num = frame[11] - 48;
        memcpy(data->MODES.ABCMODES.sensors,&frame[5],6);

        strcpy(parsed->cmd,"SCS");
        parsed->size = sizeof(tMAPS_PROTO_SC_SPECIAL);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareCAData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    if (size == 11 && isdigit(frame[4]) && isdigit(frame[5]) && isdigit(frame[6]) && isdigit(frame[7]))
    {
        tMAPS_PROTO_CA_DATA *data = (tMAPS_PROTO_CA_DATA *)calloc(1,sizeof(tMAPS_PROTO_CA_DATA));

        if (data == NULL)
            return 1;

        data->ca_sensors = ((frame[4] - 48) * 10) + (frame[5] - 48);
        data->da_sensors = ((frame[6] - 48) * 10) + (frame[7] - 48);
        parsed->size = sizeof(tMAPS_PROTO_CA_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareREData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    if (size == 7)
        return 0;
    else if (size == 39)
    {
        uint8_t pos = 4;
        tMAPS_PROTO_RE_DATA *data = (tMAPS_PROTO_RE_DATA *) calloc(1,sizeof(tMAPS_PROTO_RE_DATA));

        if (data == NULL)
            return 1;

        memcpy(data->bmodel  ,&frame[pos+1] ,K_MAPS_PROTO_BMODEL_LENGTH);
        memcpy(data->fversion,&frame[pos+11],K_MAPS_PROTO_FVERSION_LENGTH);
        memcpy(data->fnum_rev,&frame[pos+16],K_MAPS_PROTO_FNUM_REV_LENGTH);
        memcpy(data->ver_date,&frame[pos+23],K_MAPS_PROTO_VER_DATE_LENGTH);

        parsed->size = sizeof (tMAPS_PROTO_RE_DATA);
        parsed->data = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareIARMData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    if (size == 7)      // No data
        return 0;
    else if (size == 9) // Have 2 bytes in data
    {
        if (!isdigit(frame[4]) && !isdigit(frame[5]))
            return 2;
        if ((parsed->data = (char *)calloc(1,sizeof(char))) == NULL)
            return 1;

        parsed->size    = 1;
        parsed->data[0] = ((frame[4] - 48) * 10) + (frame[5] - 48);

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareFailData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_FAILURE_DATA *data;

    if (size == 10)
    {
        if (frame[4] != 'R' && frame[4] != 'E')
            return 2;
        if (!isdigit(frame[5]) || (frame[5] - 48) > 8)
            return 2;
        if (!isdigit(frame[6]) || (frame[6] - 48) > 8)
            return 2;
        if ((data = (tMAPS_PROTO_FAILURE_DATA *)calloc(1,sizeof(tMAPS_PROTO_FAILURE_DATA))) == NULL)
            return 1;

        data->type    = frame[4];
        data->ngroup  = frame[5] - 48;
        data->nsensor = frame[6] - 48;
        parsed->size  = sizeof(tMAPS_PROTO_FAILURE_DATA);
        parsed->data  = (char *) data;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareDualData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    uint8_t number;
    uint8_t fpos   = (parsed->type) ?  4 : 2; // CMF position in the frame. When response start in pos 4 on request 2
    uint16_t fsize = (parsed->type) ? 11 : 9; // The size of the frame. When response must be 9 on request 7

    if (size == fsize && isdigit(frame[fpos+2]) && isdigit(frame[fpos+3]))
    {
        number = ((frame[fpos+2] - 48) * 10) + (frame[fpos+3] - 48);

        if (!strcmp(parsed->cmd,"ER") && (number < 1 || number > 24))
            return 2;
        if (!strcmp(parsed->cmd,"SR") && (number < 3 || number > 10))
            return 2;
        if ((parsed->data = (char *)calloc(1,sizeof(char))) == NULL)
            return 1;

        parsed->size    = 1;
        parsed->data[0] = number;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareSingelData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    uint8_t fpos   = (parsed->type) ?  4 : 2; // CMF position in the frame. When response start in pos 4 on request 2
    uint16_t fsize = (parsed->type) ? 10 : 8; // The size of the frame. When response must be 9 on request 7

    if (size == fsize)
    {
        if (!strcmp(parsed->cmd,"BR") && (frame[fpos+2] < 49 || frame[fpos+2] > 53))
            return 2;
        if ((parsed->data = (char *)calloc(1,sizeof(char))) == NULL)
            return 1;

        parsed->size    = 1;
        parsed->data[0] = frame[fpos+2] - 48;

        return 0;
    }

    return 2;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareEndVehData(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_END_VEHICLE *data;

    if (size != 11 && size != 12 && size != 24)
        return 2;

    for (uint16_t i = 4, s = (size - 7) + i, c = (s > 8) ? (s - 1) : 8; i < s; i++)
    {
         if (i == c && (frame[i] != 'M' && frame[i] != 'X' && (frame[i] < 65 || frame[i] > 70)))
             return 2;
         else if (i != c && !isdigit(frame[i]))
             return 2;
    }

    if ((data = (tMAPS_PROTO_END_VEHICLE *)calloc(1,sizeof(tMAPS_PROTO_END_VEHICLE))) == NULL)
        return 1;

    // Common bytes are 4 to 7
    data->paxes  = ((frame[4] - 48) * 10) + (frame[5] - 48);
    data->naxes  = ((frame[6] - 48) * 10) + (frame[7] - 48);
    data->vclass = (size == 12) ? frame[8] : 0;
    data->smb    = (size == 11) ? 3 : 1;

    if (size == 24) // Is a CF-220 barrier message with the second SM BYTE equal to 2
    {
        data->paxes10 = ((frame[8]  - 48) * 10) + (frame[9]  - 48);
        data->naxes10 = ((frame[10] - 48) * 10) + (frame[11] - 48);
        data->paxes16 = ((frame[12] - 48) * 10) + (frame[13] - 48);
        data->naxes16 = ((frame[14] - 48) * 10) + (frame[15] - 48);
        data->paxes22 = ((frame[16] - 48) * 10) + (frame[17] - 48);
        data->naxes22 = ((frame[18] - 48) * 10) + (frame[19] - 48);
        data->vclass  = frame[20];
        data->smb     = 2;
    }

    parsed->data = (char *) data;
    parsed->size = sizeof(tMAPS_PROTO_END_VEHICLE);

    return 0;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPreparePASpecial(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_BARRIER_ADJUST *bdata;

    parsed->size = size-1;
    strcpy(parsed->cmd,"PAS");

    if ((bdata = (tMAPS_PROTO_BARRIER_ADJUST *)calloc(1,sizeof(tMAPS_PROTO_BARRIER_ADJUST))) == NULL)
        return 1;

    memcpy(bdata->rcv_map8,frame,K_MAPS_PROTO_RECEIVE_GROUP8);
    memcpy(bdata->rcv_map3,&frame[K_MAPS_PROTO_RECEIVE_GROUP8],K_MAPS_PROTO_RECEIVE_GROUP3);
    parsed->data = (char *) bdata;

    return 0;
}
//-----------------------------------------------------------------------------

uint8_t MapsProtoPrepareSCSpecial(uint8_t *frame, uint16_t size, tMAPS_PROTO_PARSED_FRAME *parsed)
{
    tMAPS_PROTO_SC_SPECIAL *scdata;
    char mode = (size == 13) ? 'D' : 'H';

    parsed->size = K_MAPS_PROTO_DEHI_BUFFER;
    strcpy(parsed->cmd,"SCS");

    if ((scdata = (tMAPS_PROTO_SC_SPECIAL *)calloc(1,sizeof(tMAPS_PROTO_SC_SPECIAL))) == NULL)
        return 1;

    scdata->mode = mode;
    memcpy(scdata->MODES.DEHI_MODES,frame,parsed->size);
    parsed->data = (char *) scdata;

    return 0;
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME * MapsProtoCreateFrame(uint8_t type, uint8_t num, const char *cmd, uint8_t *data, uint16_t data_size)
{
    const char   *types[3]  = {"","RS","NE"};
    tMAPS_PROTO_RAW_FRAME      *frame = NULL;
    const tMAPS_PROTO_CMD_INFO *cinfo = NULL;
    uint16_t lrc , pos = 2, size = (type) ? (9+data_size) : (7+data_size);

    if (type > 2 || num > 9 || !cmd || (!(cinfo = MapsProtoFindCmd(cmd)) && type != 2) || (!data && data_size))
        frame_error(EINVAL);
    if (type == 1 && !data_size && !(cinfo->suppdata & 1))
        frame_error(EINVAL);
    if (type == 0 && !data_size && !(cinfo->suppdata & 2))
        frame_error(EINVAL);
    if ((frame = (tMAPS_PROTO_RAW_FRAME *)calloc(1,sizeof(tMAPS_PROTO_RAW_FRAME))) == NULL)
        frame_error(ENOMEM);
    if ((frame->data = (uint8_t *)calloc(size,sizeof(uint8_t))) == NULL)
        frame_error(ENOMEM);

    frame->data[0] = K_MAPS_PROTO_SOH;
    frame->data[1] = num + 48;
    frame->size    = size;

    if (type) {
        memcpy(&frame->data[pos],types[type],2);
        pos += 2;
    }

    memcpy(&frame->data[pos],cmd,2);
    pos += 2;

    if (data_size) {
        memcpy(&frame->data[pos],data,data_size);
        pos += data_size;
    }

    lrc = MapsProtoCalculateLRC(&frame->data[1],size-4);
    memcpy(&frame->data[pos],&lrc,2);
    frame->data[size-1] = K_MAPS_PROTO_CR;

    return frame;
}
//-----------------------------------------------------------------------------
//############################# PUBLIC  FUNCTIONS #############################
//-------------  F R E E   R E S O U R C E S   F U N C T I O N S  -------------

void MapsProtoFreeRawFrame(tMAPS_PROTO_RAW_FRAME *raw)
{
    if (raw)
    {
        if (raw->data)
            free(raw->data);

        free(raw);
    }
}
//-----------------------------------------------------------------------------

void MapsProtoFreeParsedFrame(tMAPS_PROTO_PARSED_FRAME *parsed)
{
    if (parsed)
    {
        if (parsed->data)
            free(parsed->data);

        free(parsed);
    }
}
//-----------------------------------------------------------------------------
//----------------------  P A R S E   F U N C T I O N S  ----------------------

tMAPS_PROTO_PARSED_FRAME * MapsProtoParseFrame(uint8_t *frame, uint16_t size)
{
    uint8_t code;
    uint16_t lrc, clrc;
    uint8_t errors [] = {0, ENOMEM, ENOEXEC};
    tMAPS_PROTO_PARSED_FRAME *parsed  = NULL;
    const tMAPS_PROTO_CMD_INFO *cinfo = NULL;

    if (frame == NULL)
        parse_error(EINVAL);
    if (size < 7)                                                                                                // INVALID FRAME. Minimum size is 7
        parse_error(ESPIPE);
    if ((parsed = (tMAPS_PROTO_PARSED_FRAME *)calloc(1, sizeof (tMAPS_PROTO_PARSED_FRAME))) == NULL)
        parse_error(ENOMEM);

    if (size == K_MAPS_PROTO_PASF_SIZE+1 && frame[88] == K_MAPS_PROTO_CR)                                        // (ALL BARRIERS) PA SPECIAL 88 + <CR>
    {
        if (MapsProtoPreparePASpecial(frame,size,parsed))
            parse_error(ENOMEM);
    }
    else if ((size == K_MAPS_PROTO_SCSF_SIZE+1 && frame[12] == K_MAPS_PROTO_CR) ||                               // (CF220 & CF24P) SC SPECIAL MODES D,E 12 + <CR>
             (size == K_MAPS_PROTO_SCSF_SIZE+2 && frame[12] == K_MAPS_PROTO_CR && frame[13] == K_MAPS_PROTO_LF)) // (CF220 & CF24P) SC SPECIAL MODES H,I 12 + <CR><LF>
    {
        if (MapsProtoPrepareSCSpecial(frame,size,parsed))
            parse_error(ENOMEM);
    }
    else
    {
        parsed->num = frame[1] - 48;      // Get the frame number.
        memcpy(&lrc,&frame[size-3],2);    // Get the LRC checksum (2 bytes).
        clrc = MapsProtoCalculateLRC(&frame[1],size-4); // Calculate the checksum (LRC)

        if (lrc != clrc)
            parse_error(ERANGE);
        if (frame[0] != K_MAPS_PROTO_SOH || frame[size-1] != K_MAPS_PROTO_CR)
            parse_error(ESPIPE);
        if (parsed->num > 9)
            parse_error(EBADF);

        if (!strncmp((char *)&frame[2],"NE",2))         // ### Unknown or not Executed Message ###
        {
            parsed->type = 2; // Set frame type to NE (Unknown or not Executed).
            memcpy(parsed->cmd,&frame[4],2);

            if (size != 9)
                parse_error(EPERM);
        }
        else if (!strncmp((char *)&frame[2],"RS",2))    // ### Response Message ###
        {
            parsed->type = 1; // Set frame type to RS (Response).
            memcpy(parsed->cmd,&frame[4],2);

            if ((cinfo = MapsProtoFindCmd(parsed->cmd)) == NULL)
                parse_error(EPERM);
            if ((code = cinfo->ResponseParseFunc(frame,size,parsed)))
                parse_error(errors[code]);
        }
        else                                          // ### Request or Spontaneous Message ###
        {   
            if (!strncmp((char *)&frame[2],"FA",2) && size > 7) // Spontaneous FA request command have data. Check for it
                memcpy(parsed->cmd,"FAS",3);                    // To differentiate Spontaneous from the normal FA request we add an S at the end.
            else
                memcpy(parsed->cmd,&frame[2],2);

            if ((cinfo = MapsProtoFindCmd(parsed->cmd)) == NULL)
                parse_error(EPERM);
            if ((code = cinfo->RequestParseFunc(frame,size,parsed)))
                parse_error(errors[code]);
        }        
    }

    return parsed;

    PARSE_ERROR_EXEC:

    MapsProtoFreeParsedFrame(parsed);
    return NULL;
}
//-----------------------------------------------------------------------------
//--------------------  R E Q U E S T   F U N C T I O N S  --------------------

tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEmptyRequest(uint8_t num, const char *cmd)
{
    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,cmd,NULL,0);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateBRRequest(uint8_t num, uint8_t baud_rate)
{
    baud_rate = (baud_rate > 5) ? 49 : (baud_rate + 48);  // Baud rate must be in ASCII.
    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"BR",&baud_rate,1);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateCARequest(uint8_t num, uint8_t ncs_down, uint8_t nds_down)
{
    uint8_t data[5];

    sprintf((char *)data,"%.2u%.2u",ncs_down,nds_down);
    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"CA",data,4);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateERRequest(uint8_t num, uint8_t pcell_num)
{
    uint8_t data[3];

    if (pcell_num < 1 || pcell_num > 24)
        param_error(EINVAL);

    sprintf((char *)data,"%.2u",pcell_num);
    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"ER",data,2);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreatePRRequest(uint8_t num, uint8_t msec_time)
{
    uint8_t data[3];

    msec_time = (msec_time > 99) ? 99 : msec_time;
    sprintf((char *)data,"%.2u",msec_time);

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"PR",data,2);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateSCRequest(uint8_t num, char mode, uint16_t msec_time)
{
    uint8_t data[5];

    if (mode != 'A' && mode != 'B' && mode != 'C' &&
        mode != 'D' && mode != 'E' && mode != 'H' && mode != 'I')
        param_error(EINVAL);

    msec_time = (msec_time > 999) ? 999 : msec_time;
    sprintf((char *)data,"%c%.3u",mode,msec_time);

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"SC",data,4);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateSMRequest(uint8_t num, uint8_t elements, tMAPS_PROTO_SM_DATA *data)
{
    uint8_t buffer[6] = {0};

    if (elements < 3 || elements > 5 || !data)
        param_error(EINVAL);

    char td = (data->tow_detection == 0) ? 48 : data->tow_detection;

    if (data->work_mode > 3)
        param_error(EINVAL);
    if (data->axis_ispeed > 15)
        param_error(EINVAL);
    if (data->axis_height > 2)
        param_error(EINVAL);
    if (td != 48 && td != 'R' && td != 'M' && td != 'N' && td != 'E' && td != 'T')
        param_error(EINVAL);
    if (elements == 5 && data->rcvr_direction != 'P' && data->rcvr_direction != 'N')
        param_error(EINVAL);

    buffer[0] = data->work_mode + 48;
    buffer[1] = (data->axis_ispeed < 10) ? (data->axis_ispeed + 48) : (data->axis_ispeed + 55);
    buffer[2] = data->axis_height + 48;
    buffer[3] = td;
    buffer[4] = data->rcvr_direction;

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"SM",buffer,elements);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateSRRequest(uint8_t num, uint8_t sensors_num)
{
    uint8_t data[3];

    if (sensors_num < 3 || sensors_num > 10)
        param_error(EINVAL);

    sprintf((char *)data,"%.2u",sensors_num);

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"SR",data,2);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateRHRequest(uint8_t num, uint8_t mode, uint8_t receiver_num)
{
    uint8_t data[4];

    if (mode > 1 || !receiver_num || receiver_num > 24)
        param_error(EINVAL);

    sprintf((char *)data,"%c%.2u",mode+48,receiver_num);

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"RH",data,3);
}
//-----------------------------------------------------------------------------
//--------  S P O N T A N E O U S   R E Q U E S T   F U N C T I O N S  --------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateBarrierAdjRequest(uint8_t num, uint8_t type, tMAPS_PROTO_BARRIER_ADJUST *data)
{
    uint8_t *buffer = (uint8_t *) data;

    if (!data || sizeof(tMAPS_PROTO_BARRIER_ADJUST) != K_MAPS_PROTO_PASF_SIZE)
        param_error(EINVAL);

    for (uint8_t i = 0; i < K_MAPS_PROTO_PASF_SIZE; i++)
         if (!isxdigit(buffer[i]))
             param_error(EINVAL);

    if (type)
        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"AJ",buffer,K_MAPS_PROTO_PASF_SIZE);
    else
    {
        tMAPS_PROTO_RAW_FRAME *frame = NULL;

        if ((frame = (tMAPS_PROTO_RAW_FRAME *)calloc(1,sizeof(tMAPS_PROTO_RAW_FRAME))) == NULL)
            frame_error(ENOMEM);
        if ((frame->data = (uint8_t *)calloc(K_MAPS_PROTO_PASF_SIZE+1,sizeof(uint8_t))) == NULL)
            frame_error(ENOMEM);

        frame->size = K_MAPS_PROTO_PASF_SIZE+1;
        memcpy(frame->data,buffer,K_MAPS_PROTO_PASF_SIZE);
        frame->data[K_MAPS_PROTO_PASF_SIZE] = K_MAPS_PROTO_CR;

        return frame;
    }
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateSCSpecialRequest(uint8_t num, tMAPS_PROTO_SC_SPECIAL *data)
{
    if (!data)
        param_error(EINVAL);

    if (data->mode == 'A' || data->mode == 'B' || data->mode == 'C')
    {
        uint8_t buffer[8];

        for (uint8_t i = 0; i < K_MAPS_PROTO_SENSORS_MAP; i++)
             if (!isxdigit(data->MODES.ABCMODES.sensors[i]))
                 param_error(EINVAL);

        if (data->MODES.ABCMODES.sweeps_num > 9)
            param_error(EINVAL);

        buffer[0] = (data->MODES.ABCMODES.presence) ? 49 : 48;
        memcpy(&buffer[1],data->MODES.ABCMODES.sensors,K_MAPS_PROTO_SENSORS_MAP);
        buffer[7] = data->MODES.ABCMODES.sweeps_num + 48;

        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"SC",buffer,8);
    }
    else if (data->mode == 'D' || data->mode == 'E' || data->mode == 'H' || data->mode == 'I')
    {
        tMAPS_PROTO_RAW_FRAME *frame = NULL;
        uint8_t size = (data->mode == 'D' || data->mode == 'E') ? 1 : 2;

        for (uint8_t i = 0; i < K_MAPS_PROTO_DEHI_BUFFER; i++)
             if (!isxdigit(data->MODES.DEHI_MODES[i]))
                 param_error(EINVAL);

        if ((frame = (tMAPS_PROTO_RAW_FRAME *)calloc(1,sizeof(tMAPS_PROTO_RAW_FRAME))) == NULL)
            frame_error(ENOMEM);
        if ((frame->data = (uint8_t *)calloc(K_MAPS_PROTO_DEHI_BUFFER+size,sizeof(uint8_t))) == NULL)
            frame_error(ENOMEM);

        frame->size = K_MAPS_PROTO_DEHI_BUFFER+size;
        memcpy(frame->data,data->MODES.DEHI_MODES,K_MAPS_PROTO_DEHI_BUFFER);
        frame->data[K_MAPS_PROTO_DEHI_BUFFER] = K_MAPS_PROTO_CR;

        if (size == 2)
            frame->data[K_MAPS_PROTO_DEHI_BUFFER+1] = K_MAPS_PROTO_LF;

        return frame;
    }
    else
        param_error(EINVAL);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateAPRequest(uint8_t num, tMAPS_PROTO_AP_DATA *data)
{
    uint8_t buffer[11];

    if (!data)
        param_error(EINVAL);

    if (data->smbyte >= 2)
    {
        if (data->vaxis != 'P' && data->vaxis != 'N' && data->vaxis != 48  && data->vaxis != 0)
            param_error(EINVAL);

        buffer[0] = (data->vaxis == 0) ? 48 : data->vaxis;
        buffer[1] = 48;
        sprintf((char *)&buffer[2],"%.2u%.2u%.2u%.2u",(data->axis_height > 15) ? 15 : data->axis_height
                                                     ,(data->vmax_height > 99) ? 99 : data->vmax_height
                                                     ,(data->hmin_height > 99) ? 99 : data->hmin_height
                                                     ,(data->lmax_height > 99) ? 99 : data->lmax_height);

        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"AP",buffer,10);
    }
    else {
        sprintf((char *)buffer,"%.2u",(data->vheight > 99) ? 99 : data->vheight);
        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"AP",buffer,2);
    }
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateEJRequest(uint8_t num, tMAPS_PROTO_EJ_DATA *data)
{
    uint8_t buffer[7];

    if (!data)
        param_error(EINVAL);

    sprintf((char *)buffer,"%.2u%.2u%.2u",(data->paxes  > 99) ? 99 : data->paxes
                                         ,(data->naxes  > 99) ? 99 : data->naxes
                                         ,(data->ispeed > 99) ? 99 : data->ispeed);

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"EJ",buffer,6);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateEMRequest(uint8_t num, tMAPS_PROTO_EM_DATA *data)
{
    uint8_t buffer[11], size = 9;

    if (!data)
        param_error(EINVAL);

    char td = (data->tow_detection == 0) ? 48 : data->tow_detection;

    if (data->work_mode > 3)
        param_error(EINVAL);
    if (data->axis_ispeed > 15)
        param_error(EINVAL);
    if (data->axis_height > 2)
        param_error(EINVAL);
    if (td != 48  && td != 'R' && td != 'M' && td != 'N' && td != 'E' && td != 'T')
        param_error(EINVAL);
    if (!data->hw_failure  || data->hw_failure > 3)
        param_error(EINVAL);
    if (!data->se_cleaning || data->se_cleaning > 2)
        param_error(EINVAL);
    if (data->firmware_ver > 99)
        param_error(EINVAL);
    if (data->rcvr_direction != 0 && data->rcvr_direction != 'P' && data->rcvr_direction != 'N')
        param_error(EINVAL);

    buffer[0] = data->work_mode      + 48;
    buffer[1] = (data->axis_ispeed < 10) ? (data->axis_ispeed + 48) : (data->axis_ispeed + 55);
    buffer[2] = data->axis_height    + 48;

    if (data->rcvr_direction == 0)
    {
        buffer[3] = data->hw_failure     + 48;
        buffer[4] = data->se_cleaning    + 48;

        sprintf((char *)&buffer[5],"%.2u",data->firmware_ver);

        buffer[7] = 48;
        buffer[8] = 48;
    }
    else
    {
        size = 10;
        buffer[3] = td;
        buffer[4] = data->hw_failure     + 48;
        buffer[5] = data->se_cleaning    + 48;

        sprintf((char *)&buffer[6],"%.2u",data->firmware_ver);

        buffer[8] = data->rcvr_direction;
        buffer[9] = 48;
    }

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"EM",buffer,size);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateEndVehicleRequest(uint8_t num, uint8_t type, tMAPS_PROTO_END_VEHICLE *data)
{
    const char *cmds[] = {"FA","FR"};
    uint8_t buffer[17] , size = 4;

    if (!data || data->smb > 3)
        param_error(EINVAL);
    if (data->smb != 3 && (data->vclass != 'M' && data->vclass != 'X' && (data->vclass < 65 || data->vclass > 70)))
        param_error(EINVAL);

    type = (type) ? 1 : 0;
    sprintf((char *)buffer,"%.2u%.2u",(data->paxes > 99) ? 99 : data->paxes
                                     ,(data->naxes > 99) ? 99 : data->naxes);

    if (data->smb <= 1)
    {
        buffer[4] = data->vclass;
        size = 5;
    }
    else if (data->smb == 2)
    {
        sprintf((char *)&buffer[4],"%.2u%.2u%.2u%.2u%.2u%.2u",(data->paxes10 > 99) ? 99 : data->paxes10
                                                             ,(data->naxes10 > 99) ? 99 : data->naxes10
                                                             ,(data->paxes16 > 99) ? 99 : data->paxes16
                                                             ,(data->naxes16 > 99) ? 99 : data->naxes16
                                                             ,(data->paxes22 > 99) ? 99 : data->paxes22
                                                             ,(data->naxes22 > 99) ? 99 : data->naxes22);

        buffer[16] = data->vclass;
        size = 17;
    }

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,cmds[type],buffer,size);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateFailureRequest(uint8_t num, uint8_t type, tMAPS_PROTO_FAILURE_DATA *data)
{
    uint8_t buffer[3];
    const char *cmds[] = {"FX","PX"};

    if (!data || (data->type != 'R' && data->type != 'E') || data->ngroup > 8 || data->nsensor > 8)
        param_error(EINVAL);

    type = (type) ? 1 : 0;
    buffer[0] = data->type;
    buffer[1] = data->ngroup  + 48;
    buffer[2] = data->nsensor + 48;

    return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,cmds[type],buffer,3);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateIARequest(uint8_t num, uint8_t ispeed)
{
    uint8_t buffer[3];

    ispeed = (ispeed > 99) ? 99 : ispeed;
    sprintf((char *)buffer,"%.2u",ispeed);

    if (ispeed)
        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"IA",buffer,2);
    else
        return MapsProtoCreateEmptyRequest(num,"IA");

}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME * MapsProtoCreateRERequest(uint8_t num, uint8_t firm_ver, uint8_t rev_ver, uint32_t date_ver)
{
    uint8_t date[3];    // Saved as aammdd
    uint8_t buffer[33];
    uint8_t days[] = {31,29,31,30,31,30,31,31,30,31,30,31};

    if (firm_ver && rev_ver && date_ver)
    {
          firm_ver = (firm_ver > 99) ? 99 : firm_ver;
          rev_ver  = (rev_ver  > 99) ? 99 : rev_ver;

          for (uint8_t i = 0; i < 3; i++)
          {
              date[i] = date_ver % 100;
              if (i == 1 && (!date[i] || date[i] > 12))              // Month
                  param_error(EINVAL);
              if (i == 2 && (!date[i] || date[i] > days[date[1]-1])) // Day
                  param_error(EINVAL);

              date_ver /= 100;
          }

          sprintf((char *)buffer,"/32CF-220M/V-%.2u/R-%.2u/D-%.2u-%.2u-%.2u/",firm_ver,rev_ver,date[2],date[1],date[0]);

        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"RE",buffer,32);
    }
    else
        return MapsProtoCreateEmptyRequest(num,"RE");
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateRMRequest(uint8_t num, uint8_t naxes)
{
    uint8_t buffer[3];

    naxes = (naxes > 99) ? 99 : naxes;
    sprintf((char *)buffer,"%.2u",naxes);

    if (naxes)
        return MapsProtoCreateFrame(K_MAPS_PROTO_REQ_TYPE,num,"RM",buffer,2);
    else
        return MapsProtoCreateEmptyRequest(num,"RM");
}
//-----------------------------------------------------------------------------
//-------------------  R E S P O N S E   F U N C T I O N S  -------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateUnknownResponse(uint8_t num, const char *cmd)
{
    return MapsProtoCreateFrame(K_MAPS_PROTO_UNK_TYPE,num,cmd,NULL,0);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateEmptyResponse(uint8_t num, const char *cmd)
{
    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,cmd,NULL,0);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateDEResponse(uint8_t num, tMAPS_PROTO_DE_DATA *data)
{
    uint8_t buffer[10] = {0};

    if (!data)
        param_error(EINVAL);

    char td = (data->tow_detection == 0) ? 48 : data->tow_detection;

    if (data->work_mode > 3)
        param_error(EINVAL);
    if (data->axis_ispeed > 15)
        param_error(EINVAL);
    if (data->axis_height > 2)
        param_error(EINVAL);
    if (td != 48  && td != 'R' && td != 'M' && td != 'N' && td != 'E' && td != 'T')
        param_error(EINVAL);
    if (!data->hw_failure  || data->hw_failure > 3)
        param_error(EINVAL);
    if (!data->se_cleaning || data->se_cleaning > 2)
        param_error(EINVAL);
    if (data->firmware_ver > 99)
        param_error(EINVAL);
    if (data->rcvr_direction != 0 && data->rcvr_direction != 'P' && data->rcvr_direction != 'N')
        param_error(EINVAL);

    buffer[0] = data->work_mode      + 48;
    buffer[1] = (data->axis_ispeed < 10) ? (data->axis_ispeed + 48) : (data->axis_ispeed + 55);
    buffer[2] = data->axis_height    + 48;
    buffer[3] = td;
    buffer[4] = data->hw_failure     + 48;
    buffer[5] = data->se_cleaning    + 48;

    sprintf((char *)&buffer[6],"%.2u",data->firmware_ver);

    buffer[8] = (data->rcvr_direction == 0) ? (data->rcvr_direction + 48) : data->rcvr_direction;
    buffer[9] = (data->barrier_model  < 10) ? (data->barrier_model  + 48) : data->barrier_model;

    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"DE",buffer,10);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateEAResponse(uint8_t num, tMAPS_PROTO_EA_DATA *data)
{
    uint8_t buffer[9];

    if (!data)
        param_error(EINVAL);

    sprintf((char *)buffer,"%.2u%.2u%.2u%.2u",data->imax_height,data->umax_height,data->umin_height,data->lmax_height);

    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"EA",buffer,8);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateERResponse(uint8_t num, uint8_t recv_status)
{
    recv_status = (recv_status) ? 49 : 48;
    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"ER",&recv_status,1);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateTTResponse(uint8_t num, tMAPS_PROTO_TT_DATA *data)
{
    uint8_t buffer[26];

    if (!data)
        param_error(EINVAL);

    for (uint8_t i = 0; i < K_MAPS_PROTO_EMITTERS_MAP_SIZE; i++)
        if (!isxdigit(data->e_map[i]))
            param_error(EINVAL);

    for (uint8_t i = 0; i < K_MAPS_PROTO_RECEIVERS_MAP_SIZE; i++)
        if (!isxdigit(data->r_map[i]))
            param_error(EINVAL);

    buffer[0]  = 'M';
    buffer[17] = 'R';
    memcpy(&buffer[1] ,data->e_map,K_MAPS_PROTO_EMITTERS_MAP_SIZE);
    memcpy(&buffer[18],data->r_map,K_MAPS_PROTO_RECEIVERS_MAP_SIZE);

    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"TT",buffer,26);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateRHResponse(uint8_t num, uint8_t wmode, uint8_t recvn)
{
    uint8_t buffer[4];

    if (recvn < 1 || recvn > 24)
        param_error(EINVAL);

    wmode = (wmode) ? 49 : 48;
    sprintf((char *)buffer,"%c%.2u",wmode,recvn);

    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"RH",buffer,3);
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME *MapsProtoCreateCBResponse(uint8_t num, uint8_t loop_state)
{
    loop_state = (loop_state) ? 49 : 48;
    return MapsProtoCreateFrame(K_MAPS_PROTO_RES_TYPE,num,"CB",&loop_state,1);
}
//-----------------------------------------------------------------------------
