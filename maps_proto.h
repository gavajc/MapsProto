#ifndef MAPS_PROTO_H
#define MAPS_PROTO_H
//-----------------------------------------------------------------------------

/** @file maps_proto.h
 *  @author Juan Carlos García Vázquez (gavajc)
 *  @date 2022-02-15
 *  @brief Function prototypes for manipulate MAPS frames, create
 *         MAPS messages and parse MAPS responses.
 *
 *  The MAPS library was developed to facilitate interaction with MAPS
 *  optical barrirers: CF-24P, CF-150 and CF-220/M
 */

#include <errno.h>
#include <stdint.h>
//-----------------------------------------------------------------------------

#define K_MAPS_PROTO_CMD_LENGTH         3
#define K_MAPS_PROTO_EMITTERS_MAP_SIZE  16
#define K_MAPS_PROTO_RECEIVERS_MAP_SIZE 8
#define K_MAPS_PROTO_RECEIVE_GROUP8     64
#define K_MAPS_PROTO_RECEIVE_GROUP3     24
#define K_MAPS_PROTO_SENSORS_MAP        6
#define K_MAPS_PROTO_DEHI_BUFFER        12
#define K_MAPS_PROTO_BMODEL_LENGTH      9
#define K_MAPS_PROTO_FVERSION_LENGTH    4
#define K_MAPS_PROTO_FNUM_REV_LENGTH    4
#define K_MAPS_PROTO_VER_DATE_LENGTH    8
//-----------------------------------------------------------------------------

/**
 *
 * @struct tMAPS_PROTO_RAW_FRAME
 * @brief  A MAPS FRAME that can be a request or response in a RAW format.
 *
 */
typedef struct
{
    uint16_t size;        ///< The size of the data.
    uint8_t *data;        ///< The MAPS DATA in RAW format.
}tMAPS_PROTO_RAW_FRAME;

/**
 *
 * @struct tMAPS_PROTO_PARSED_FRAME
 * @brief  Represent elements of the MAPS frame.
 *
 *         When TYPE is 2 the command not was executed or is Unknown then
 *         data is NULL and size is zero.
 *
 *         For a reequest (TYPE=0) or response (TYPE=1) and data is not NULL
 *         and size is not zero, data must be casting acording to the cmd value.
 *
 *         The FA, PA and SC have spontaneous commands so in cmd appears as:
 *
 *         FA ---> FAS; PA ---> PAS; SC ---> SCS
 *
 *         The complete list of conversions is:
 *
 *                               IS A REQUEST                   IS A RESPONSE               COMPATIBILITY
 *     |     CMD     |     (TYPE=0) CAST DATA TO     |     (TYPE=1) CAST DATA TO     |  CF-220 CF-150 CF-24P
 *     -------------- ------------------------------- ------------------------------- ----------------------
 *           BR                NONE IS 1 BYTE                  NONE IS NULL               1      0      1
 *           CA             tMAPS_PROTO_CA_DATA                NONE IS NULL               1      0      0
 *           DE                 NONE IS NULL               tMAPS_PROTO_DE_DATA            1      1      1
 *           EA                 NONE IS NULL               tMAPS_PROTO_EA_DATA            1      0      1
 *           ER                NONE IS 1 BYTE                 NONE IS 1 BYTE              1      0      1
 *           FA                 NONE IS NULL                   NONE IS NULL               1      1      1
 *           MV                 NONE IS NULL                   NONE IS NULL               1      1      1
 *           PA                 NONE IS NULL                   NONE IS NULL               1      1      1
 *           AC                 NONE IS NULL                   NONE IS NULL               1      1      1
 *           PR                NONE IS 1 BYTE                  NONE IS NULL               1      0      0
 *           RF                 NONE IS NULL                   NONE IS NULL               1      1      1
 *           SC             tMAPS_PROTO_SC_DATA                NONE IS NULL               1      0      1
 *           SM             tMAPS_PROTO_SM_DATA                NONE IS NULL               1      1      1
 *           SR                NONE IS 1 BYTE                 NONE IS 1 BYTE              1      0      0
 *           TT                 NONE IS NULL               tMAPS_PROTO_TT_DATA            1      1      1
 *           RH             tMAPS_PROTO_RH_DATA            tMAPS_PROTO_RH_DATA            0      0      1
 *           CB                 NONE IS NULL                  NONE IS 1 BYTE              0      1      0
 *
 *     -------------- ------------------------------- ------------------------------- ----------------------
 *         S    P    O    N    T    A    N    E    O    U    S      C    O    M    M    A    N    D    S
 *     -------------- ------------------------------- ------------------------------- ----------------------
 *
 *           AJ         tMAPS_PROTO_BARRIER_ADJUST            NONE IS NULL                1      1      1
 *       PA SPECIAL     tMAPS_PROTO_BARRIER_ADJUST            NONE IS NULL                1      1      1
 *       SC SPECIAL       tMAPS_PROTO_SC_SPECIAL              NONE IS NULL                1      0      1
 *           AP            tMAPS_PROTO_AP_DATA                NONE IS NULL                1      1      1
 *           EJ            tMAPS_PROTO_EJ_DATA                NONE IS NULL                1      0      0
 *           EM            tMAPS_PROTO_EM_DATA                NONE IS NULL                1      1      1
 *           FP                 NONE IS NULL                  NONE IS NULL                0      0      1
 *           FA           tMAPS_PROTO_END_VEHICLE             NONE IS NULL                1      1      0
 *           FR           tMAPS_PROTO_END_VEHICLE             NONE IS NULL                1      1      0
 *           FX          tMAPS_PROTO_FAILURE_DATA             NONE IS NULL                1      1      1
 *           IP                 NONE IS NULL                  NONE IS NULL                0      0      1
 *
 *           IA            NONE IS NULL ON CF150              NONE IS NULL                1      1      0
 *                        NONE IS 1 BYTE ON CF220
 *
 *           IR                 NONE IS NULL                  NONE IS NULL                1      1      0
 *           PX          tMAPS_PROTO_FAILURE_DATA             NONE IS NULL                1      1      1
 *
 *           RE         tMAPS_PROTO_RE_DATA FOR CF220,        NONE IS NULL                1      1      1
 *                      NONE IS NULL FOR CF150 & CF24P
 *
 *           RM      NONE IS NULL ON CF150. NONE IS NULL      NONE IS NULL                1      1      0
 *                   ON CF220 OR NONE BECAUSE IS 1 BYTE
 *
 */
typedef struct
{
    uint8_t num;          ///< Number between 0-9
    uint8_t type;         ///< Type of frame: 0: Request. 1: Response. 2: Unknown MSG or Not Executed.
    char cmd[K_MAPS_PROTO_CMD_LENGTH+1]; ///< The Command. SCS = SC Special, PAS = PA Special and FAS = FA Spontaneous. Is a NULL terminate string
    uint16_t size;        ///< The size of the data field.
    char *data;           ///< The data in the frame.
}tMAPS_PROTO_PARSED_FRAME;

/**
 *
 * @struct tMAPS_PROTO_CA_DATA
 * @brief  Represent elements of the CA DATA message (SET NUMBER MAX. ANOMALIES).
 *
 */
typedef struct
{
    uint8_t ca_sensors;   ///< Number of sensors disabled to generate cleaning alarm. Default 01
    uint8_t da_sensors;   ///< Number of disabled sensors to generate a degradation alarm. Default 02
}tMAPS_PROTO_CA_DATA;

/**
 *
 * @struct tMAPS_PROTO_DE_DATA
 * @brief  Have the barrier status data.
 *
 *      The data information is:
 *
 *      ### WORK_MODE ###
 *          0 = Cleaning
 *          1 = Inactive
 *          2 = Active
 *          3 = Active with clasification msg [Only CF220]
 *
 *      ### AXIS_ISPEED ###
 *          [CF24P] NOT USED
 *          [CF150] Num. AXES. 0=OFF 1=ON
 *          [CF220]
 *                 0: Axis detection is not sent.
 *                 1: Axis detection activated.
 *                 2: Axis transition detection and message in the 4 lower sensors.
 *                 4: Measurement of the instantaneous speed at the start of the advance.
 *                 8: Message of the number of axes and speed in each axis detection
 *                 If you require a combination do the sum. For example: 1 + 4 = 5
 *
 *      ### AXIS_HEIGHT ###
 *          [CF24P]
 *                  0: Sends the height message on the first axis.
 *                  2: Height detection Msg at the start of the vehicle, at each axle
 *                                      and at the end of the vehicle; max, min, top
 *                                      and bottom height of the vehicle
 *
 *          [CF150] Vehicle height on 1st axle. 0=OFF 1=ON
 *          [CF220]
 *                   0: The height message is not sent.
 *                   1: Height message on the first positive axis activated.
 *                   2: Equal to the value in CF24P.
 *
 *      ### TOW_DETECTION ###
 *          [CF24P] NOT USED
 *          [CF150] Active tow. 0=OFF R=ON
 *          [CF220]
 *                  0: Indicates tow and motorcycle OFF (default)
 *                  A: Tow ON
 *                  M: Motorcycle ON
 *                  N: Tow ON and motorcycle ON
 *                  E: Tow ON with axles detected to the tow
 *                  T: Same as option E and motorcycle ON
 *
 *       ### HW_FAILURE ###
 *          1: Sensors OK
 *          2: Some sensors fail, it is still operational, it is recommended to send a curtain test command to see the sensor map.
 *          3: Status of curtain degraded, it is not operational and requires maintenance
 *
 *       ### SE_CLEANING ###
 *          1: No cleaning required
 *          2: Precise cleaning
 *
 *       ### RCVR_DIRECTION ###
 *       Only used in CF-220 barriers
 *          P: Receiver shaft on the left in the direction of travel of the vehicle.
 *          N: Receiver col to the right in the direction of vehicle travel.
 *
 */
typedef struct
{
    uint8_t work_mode;    ///< Barrier working mode.
    uint8_t axis_ispeed;  ///< The axis information or instant speed.
    uint8_t axis_height;  ///< Vehicle height on the first axle.
    char  tow_detection;  ///< Active motorcycle and trailer detection.
    uint8_t hw_failure;   ///< Hardware fault detection.
    uint8_t se_cleaning;  ///< Detection of contamination in sensors.
    uint8_t firmware_ver; ///< Firmware version. 11 is equal to v1.1.
    char rcvr_direction;  ///< Only with CF-220 barriers.
    char barrier_model;   ///< Only with CF-220 barriers. Indicates barrier model. 4: Indicates CF-220/CF220M.
}tMAPS_PROTO_DE_DATA;

/**
 *
 * @struct tMAPS_PROTO_EA_DATA
 * @brief  The data in a EA response (STATE HEIGHTS).
 *
 */
typedef struct
{
    uint8_t imax_height;  ///< Instantaneous maximum height. Range 00 to 99
    uint8_t umax_height;  ///< Upper maximum height from previous AP frame or previous RSEA frame (decimetres).  Range 00 to 99
    uint8_t umin_height;  ///< Minimum height of top from previous AP frame or previous RSEA frame (decimetres). Range 00 to 99
    uint8_t lmax_height;  ///< Maximum height of the vehicle underbody from the previous AP frame or the previous RSEA frame (centimeters) Range 00 to 99.
}tMAPS_PROTO_EA_DATA;

/**
 *
 * @struct tMAPS_PROTO_SC_DATA
 * @brief  The data in a SC message request (SCANNER MODE).
 *
 *         The availables mode are:
 *
 *         ONLY ON CF-24P and 24 horizontal receivers
 *          A: Only when are diferences
 *          B: Continuously
 *          C: Continuously with presence
 *
 *                             CF-24P                   \           CF220M
 *          D: Continuously                             \     if there is change
 *          E: Only if there is a vehicle present       \     continuously.
 *          H: Continuously (LF)                        \     if there is a change
 *          I: Only if there is vehicle presence (LF)   \     continuously.
 *
 *        For send_time the minimum time is 5ms when the baud rate is
 *        configured as 115200 bps or 30ms when baudrate is 9600 bps
 *
 */
typedef struct
{
    char mode;            ///< Working mode. See description for details (A,B,C,D,E,H,I).
    uint16_t send_time;   ///< The time in milliseconds for the barrier send the data.
}tMAPS_PROTO_SC_DATA;

/**
 *
 * @struct tMAPS_PROTO_SM_DATA
 * @brief  Have the barrier working mode data. For request SM message.
 *
 *      The data information is:
 *
 *      ### WORK_MODE ###
 *          0 = Cleaning
 *          1 = Inactive
 *          2 = Active
 *          3 = Active with clasification msg [Only CF220]
 *
 *      ### AXIS_ISPEED ###
 *          [CF24P] NOT USED
 *          [CF150] Num. AXES. 0=OFF 1=ON
 *          [CF220]
 *                 0: Axis detection is not sent.
 *                 1: Axis detection activated.
 *                 2: Axis transition detection and message in the 4 lower sensors.
 *                 4: Measurement of the instantaneous speed at the start of the advance.
 *                 8: Message of the number of axes and speed in each axis detection
 *                 If you require a combination do the sum. For example: 1 + 4 = 5
 *
 *      ### AXIS_HEIGHT ###
 *          [CF24P]
 *                  0: Sends the height message on the first axis.
 *                  2: Height detection Msg at the start of the vehicle, at each axle
 *                                      and at the end of the vehicle; max, min, top
 *                                      and bottom height of the vehicle
 *
 *          [CF150] Vehicle height on 1st axle. 0=OFF 1=ON
 *          [CF220]
 *                   0: The height message is not sent.
 *                   1: Height message on the first positive axis activated.
 *                   2: Equal to the value in CF24P.
 *
 *      ### TOW_DETECTION ###
 *          [CF24P] NOT USED
 *          [CF150] Active tow. 0=OFF R=ON
 *          [CF220]
 *                  R: Tow ON
 *                  M: Motorcycle ON
 *                  N: Tow ON and motorcycle ON
 *                  E: Tow ON with axles detected to the tow
 *                  T: Same as option E and motorcycle ON
 *                  0: Indicates tow and motorcycle OFF (default)
 *
 *       ### RCVR_DIRECTION ###
 *       Only used in CF-220 barriers
 *          P: Receiver shaft on the left in the direction of travel of the vehicle.
 *          N: Receiver col to the right in the direction of vehicle travel.
 *
 */
typedef struct
{
    uint8_t work_mode;    ///< Barrier working mode.
    uint8_t axis_ispeed;  ///< The axis information or instant speed.
    uint8_t axis_height;  ///< Vehicle height on the first axle.
    char  tow_detection;  ///< Active motorcycle and trailer detection. Only with CF-220 & CF-150
    char rcvr_direction;  ///< Receiver column direction. Only with CF-220 barriers.
}tMAPS_PROTO_SM_DATA;

/**
 *
 * @struct tMAPS_PROTO_TT_DATA
 * @brief  The data in a TT response (BARRIER TEST).
 *
 *       EMAP: <transmitter status map>
 *          In total there can be a maximum of 8 groups of 8 transmitters (6 groups for CF24P).
 *          Each group is represented by 2 bytes, where each byte represents the 16 possible
 *          combinations for the state of four emitters.
 *
 *          Each byte is indicated by a value between 0 and 9, and A and F.
 *
 *          The F value corresponds to the four emitters in good condition.
 *          The value 0 corresponds to the four emitters in poor condition.
 *
 *          The last 4 bytes are not used in CF24P BARRIER (12 to 15)
 *
 *       RMAP: <receivers status map>
 *          In total there can be a maximum of 8 groups of 4 receivers (6 groups in CF24P).
 *          Each group is represented by 1 byte, where each byte represents the 16 possible
 *          combinations for the state of four receivers.
 *
 *          Each byte is indicated by a value between 0 and 9, and A and F.
 *
 *          The F value corresponds to the four receivers in good condition.
 *          The value 0 corresponds to the four receivers in poor condition
 *
 *          The last 2 bytes are not used in CF24P BARRIER (6 and 7)
 *
 */
typedef struct
{
    uint8_t mvar;         ///< Always have the letter M
    char e_map[K_MAPS_PROTO_EMITTERS_MAP_SIZE];  ///< The emitters map. Each byte have an hex value in ASCII: 0-9 and A-F. See description
    uint8_t rvar;         ///< Always have the letter R
    char r_map[K_MAPS_PROTO_RECEIVERS_MAP_SIZE]; ///< The receivers map. Each byte have an hex value in ASCII: 0-9 and A-F. See description
}tMAPS_PROTO_TT_DATA;

/**
 *
 * @struct tMAPS_PROTO_RH_DATA
 * @brief  The data in a RH response (STATE HEIGHTS).
 *         When wmode is 1 then closes a contact by cutting the corresponding sensor beam.
 *
 */
typedef struct
{
    uint8_t wmode;        ///< The working mode. 0: Height detection on the first axis. 1: Acts as a photocell.
    uint8_t recvn;        ///< NN: Receiver number for height or photocell detection.
}tMAPS_PROTO_RH_DATA;

/**
 *
 * @struct tMAPS_PROTO_BARRIER_ADJUST
 * @brief  The data in a AJ and PA SPECIAL messages (BARRIER ADJUST).
 *         AJ and PA SPECIAL is supported by all barriers families but
 *         the frame_end is ONLY used with the command PA SPECIAL with
 *         the CF24P & CF-150 barriers.
 *
 *          THE VALUES MUST BE IN ASCII.
 *
 *         Each receiver is represented by 2 bytes, where each byte represents
 *         the 16 possible combinations for the reception of four transmitters.
 *         Each byte is indicated by a value between 0 and 9, and A and F.
 *
 *         The F value corresponds to the four emitters in good condition.
 *         The value 0 corresponds to the four emitters in poor condition.
 *
 */
typedef struct
{
    char rcv_map8[K_MAPS_PROTO_RECEIVE_GROUP8]; ///< The receive map. 8 groups and 6 for CF24P. The last 16 bytes (48 to 63) are reserved on CF24P
    char rcv_map3[K_MAPS_PROTO_RECEIVE_GROUP3]; ///< The receive map for the 3 groups. See the possible values in the description of the structure.
}tMAPS_PROTO_BARRIER_ADJUST;

/**
 *
 * @struct tMAPS_PROTO_SC_SPECIAL
 * @brief  The data in a SC SPECIAL responsees (SCANNER MODE).
 *         When use D,E,H or I mode you must use DEHI_MODES array in
 *         the anonymous union. But if the member mode is A,B, or C then
 *         you must use the ABCMODES structure.
 *
 *         When mode is A,B or C the values of the sensors array in ABCMODES structure
 *         have the range 000000 to FFFFFF:
 *
 *         010000 indicates concealment of receiver 1 from the bottom.
 *         000080 indicates concealment of the 24 last sensor from the top.
 *
 *         The values for D,E,H,I modes are:
 *
 *         <map with the reception status of the 48 transmitters>
 *              Each byte represents the 16 possible combinations for the reception
 *              of four transmitters. Value between 0 and 9, and A and F. The first
 *              byte of the frame corresponds to the top 4 sensors, the second to
 *              the next 4 and so on.
 *
 *              The value F corresponds to the four hidden emitters.
 *              The value 0 indicates the correct reception of the 4 corresponding transmitters.
 *              The generated frame is a 12-character string with a trailing <CR> for modes D,E and <CR><LF> for H,I."
 */
typedef struct
{
    uint8_t mode;               ///< Indicate the mode used for store data. If mode is A,B or C then use the structure otherwise use the array in the union.
    union
    {
        struct
        {
            uint8_t presence;   ///< Indicates presence. 0=Not presence 1=Vehicle presence
            char sensors[K_MAPS_PROTO_SENSORS_MAP]; ///< Sensors bit map for the 24 sesnors. Range 000000 to FFFFFF. ASCII HEX.
            uint8_t sweeps_num; ///< Indicates the number of times that sweeps repeated. Range 0 to 9
        } ABCMODES;
        char DEHI_MODES[K_MAPS_PROTO_DEHI_BUFFER];  ///< Emitters reception map (48 emitters). ASCII HEX.
    } MODES;
}tMAPS_PROTO_SC_SPECIAL;

/**
 *
 * @struct tMAPS_PROTO_AP_DATA
 * @brief  The data in a AP response (HEIGHT ABOVE THE FIRST POSITIVE AXIS).
 *
 */
typedef struct
{
    uint8_t smbyte;       ///< Indicates the available values. 0 or 1 Only have vheight. 2 or higher all values are available except vheight.
    uint8_t vheight;      ///< Vehicle height. 14 indicates 1.4 mts. With CF-150 is always available. With CF24P & CF220 the third SM BYTE must be 0 or 1 respectively.
    uint8_t vaxis;        ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Vehicle axis. P=Positive N=Negative On CF24P Always is 0.
    uint8_t reserved;     ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Not used Always is 0.
    uint8_t axis_height;  ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Vehicle height on the axle (decimeters, maximum 15)
    uint8_t vmax_height;  ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Maximum height of the vehicle, from the start to the axle or between axles (decimetres).
    uint8_t hmin_height;  ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Minimum height of the top, from the beginning to the axis or between axes (decimeters)
    uint8_t lmax_height;  ///< Only used by CF220 & CF24P when the third SM BYTE is 2. Maximum height of the underbody of the vehicle, from the beginning or between axles (centimeters, maximum 99)
}tMAPS_PROTO_AP_DATA;

/**
 *
 * @struct tMAPS_PROTO_EJ_DATA
 * @brief  The data in a EJ response (NUMBER OF AXES AND SPEED WHEN DETECTING AN AXIS).
 *
 */
typedef struct
{
    uint8_t paxes;        ///< The positive axes. Range 00 to 99
    uint8_t naxes;        ///< The negative axes. Range 00 to 99
    uint8_t ispeed;       ///< Instantaneous speed on the axis. In Km/h (from 00 to 99).
}tMAPS_PROTO_EJ_DATA;

/**
 *
 * @struct tMAPS_PROTO_EM_DATA
 * @brief  Have the barrier status data by failure.
 *
 *      The data information is:
 *
 *      ### WORK_MODE ###
 *          0 = Cleaning
 *          1 = Inactive
 *          2 = Active
 *          3 = Active with clasification msg [Only CF220]
 *
 *      ### AXIS_ISPEED ###
 *          [CF24P] NOT USED
 *          [CF150] Num. AXES. 0=OFF 1=ON
 *          [CF220]
 *                 0: Axis detection is not sent.
 *                 1: Axis detection activated.
 *                 2: Axis transition detection and message in the 4 lower sensors.
 *                 4: Measurement of the instantaneous speed at the start of the advance.
 *                 8: Message of the number of axes and speed in each axis detection
 *                 If you require a combination do the sum. For example: 1 + 4 = 5
 *
 *      ### AXIS_HEIGHT ###
 *          [CF24P]
 *                  0: Sends the height message on the first axis.
 *                  2: Height detection Msg at the start of the vehicle, at each axle
 *                                      and at the end of the vehicle; max, min, top
 *                                      and bottom height of the vehicle
 *
 *          [CF150] Vehicle height on 1st axle. 0=OFF 1=ON
 *          [CF220]
 *                   0: The height message is not sent.
 *                   1: Height message on the first positive axis activated.
 *                   2: Equal to the value in CF24P.
 *
 *      ### TOW_DETECTION ###
 *          [CF24P] NOT USED
 *          [CF150] NOT USED
 *          [CF220]
 *                  0: Indicates tow and motorcycle OFF (default)
 *                  R: Tow ON
 *                  M: Motorcycle ON
 *                  N: Tow ON and motorcycle ON
 *                  E: Tow ON with axles detected to the tow
 *                  T: Same as option E and motorcycle ON
 *
 *       ### HW_FAILURE ###
 *          1: Sensors OK
 *          2: Some sensors fail, it is still operational, it is recommended to send a curtain test command to see the sensor map.
 *          3: Status of curtain degraded, it is not operational and requires maintenance
 *
 *       ### SE_CLEANING ###
 *          1: No cleaning required
 *          2: Precise cleaning
 *
 *       ### RCVR_DIRECTION ###
 *       Only used in CF-220 barriers
 *          P: Receiver shaft on the left in the direction of travel of the vehicle.
 *          N: Receiver col to the right in the direction of vehicle travel.
 *
 */
typedef struct
{
    uint8_t work_mode;    ///< Barrier working mode.
    uint8_t axis_ispeed;  ///< The axis information or instant speed.
    uint8_t axis_height;  ///< Vehicle height on the first axle.
    char  tow_detection;  ///< Only with CF-220 barriers. Active motorcycle and trailer detection.
    uint8_t hw_failure;   ///< Hardware fault detection.
    uint8_t se_cleaning;  ///< Detection of contamination in sensors.
    uint8_t firmware_ver; ///< Firmware version. 11 is equal to v1.1
    char rcvr_direction;  ///< Only with CF-220 barriers. Must be P or N. If 0, all CF-220 members will be unavailable and will be 0.
    char reserved;        ///< Only with CF-220 barriers. Not used. Is reserved for future use. Always 0
}tMAPS_PROTO_EM_DATA;

/**
 *
 * @struct tMAPS_PROTO_END_VEHICLE
 * @brief  The data in a FA/FR message (END PRESENCE VEHICLE MOVING FORWARD/BACKWARD).
 *
 *         smb is the SMBYTE number 2 used by the BARRIER CF-220 ONLY with the values 0,1 or 2
 *         smb not exists in the BARRIER CF-150 so when is 3 only PAXES & NAXES will be available.
 *         The values for clasification byte (class) are:
 *
 *              M: Motorcycle
 *              A: 2-axle vehicle, height on the first axle less than 1.5m
 *              B: 2-axle vehicle plus trailer, height on the first axle less than 1.5m
 *              C: Vehicle with more than 2 axles, with height less than 1.5m in the first axle
 *              D: 2-axle vehicle, height on the first axle greater than 1.5m
 *              E: 3-axle vehicle, height on the first axle greater than 1.5m
 *              F: Vehicle with more than 3 axles, height on the first axle greater than 1.5m
 *
 *          CF-150 not use de SM BYTE. On CF-220 if the second SM BYTE is equal to 0 or 1 then
 *          only have the PAXES (positive axes), NAXES (negative axes) and CLASS (clasification byte).
 */
typedef struct
{
    uint8_t smb;          ///< The SM BYTE that indicate what members are available. 0 or 1: PAXES, NAXES AND VCLASS. 2: All members 3: PAXES & NAXES
    char vclass;          ///< The classification byte. See available values in struct description.
    uint8_t paxes;        ///< The positive axes. Range 00 to 99
    uint8_t naxes;        ///< The negative axes. Range 00 to 99
    uint8_t paxes10;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of positive axes at 10cm from the base of the barrier. Range 00 to 99
    uint8_t naxes10;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of negative axes at 10cm from the base of the barrier. Range 00 to 99
    uint8_t paxes16;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of positive axes at 16cm from the base of the barrier. Range 00 to 99
    uint8_t naxes16;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of negative axes at 16cm from the base of the barrier. Range 00 to 99
    uint8_t paxes22;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of positive axes at 22cm from the base of the barrier. Range 00 to 99
    uint8_t naxes22;      ///< ONLY CF-220. If the second SM BYTES is 2. No. of negative axes at 22cm from the base of the barrier. Range 00 to 99
}tMAPS_PROTO_END_VEHICLE;

/**
 *
 * @struct tMAPS_PROTO_FAILURE_DATA
 * @brief  The data in a failure message. FX/PX (FAILURE START/FAILURE END).
 *
 */
typedef struct
{
    uint8_t type;         ///< The type of the affected sensor. R=Receiver E=Emitter
    uint8_t ngroup;       ///< The number of the group with the failure. Range: 1 to 6 (CF24P). 1 to 8 (CF220 & CF150).
    uint8_t nsensor;      ///< The number of sensor. 1 to 4 for receiver and 1 to 8 for emitter.
}tMAPS_PROTO_FAILURE_DATA;

/**
 *
 * @struct tMAPS_PROTO_RM_DATA
 * @brief  The data in a RM message for CF-220 barriers. All strings are NULL terminate.
 *
 */
typedef struct
{
    char bmodel[K_MAPS_PROTO_BMODEL_LENGTH+1];     ///< Barrier model.            Is a NULL terminate string. Example: 32CF-220M
    char fversion[K_MAPS_PROTO_FVERSION_LENGTH+1]; ///< Formware version.         Is a NULL terminate string. Example V-30
    char fnum_rev[K_MAPS_PROTO_FNUM_REV_LENGTH+1]; ///< Firmware number revision. Is a NULL terminate string. Example: R-01
    char ver_date[K_MAPS_PROTO_VER_DATE_LENGTH+1]; ///< Revision date:            Is a NULL terminate string. Example: 03-02-21
}tMAPS_PROTO_RE_DATA;

// Free & Parse Functions
//-----------------------------------------------------------------------------

/** @brief Free tMAPS_PROTO_RAW_FRAME structure.
 *
 * @param  raw The RAW MAPS frame to free. Previously generated with some MAPS request or response function.
 *
 */
void MapsProtoFreeRawFrame(tMAPS_PROTO_RAW_FRAME *raw);

/** @brief Free tMAPS_PROTO_PARSED_FRAME structure.
 *
 * @param  parsed The parsed MAPS frame to free. Previously created with MapsProtoParseFrame fucntion.
 *
 */
void MapsProtoFreeParsedFrame(tMAPS_PROTO_PARSED_FRAME *parsed);

/** @brief Validate and Parse a MAPS frame into tMAPS_PROTO_PARSED_FRAME structure.
 *
 *  The errno values are:
 *
 *       ENOMEM: Couldn't allocate memory
 *       EINVAL: The argument msg is an invalid pointer. i.e. NULL
 *        EPERM: Unknown, unsupported MAPS command or is not a valid response value (RE or NE).
 *        EBADF: The frame number is not in range (out of range 0 to 9).
 *       ESPIPE: The frame structure is invalid. i.e Not have SOH or CR byte or the frame have an invalid structure.
 *       ERANGE: The frame checksum is invalid.
 *      ENOEXEC: The frame data section has an invalid structure or an invalid value.
 *
 * @param  frame The MAPS frame to parse.
 * @param  size  The message size.
 * @return NULL on error and Errno is set or on sucess a new allocated tMAPS_PROTO_PARSED_FRAME structure.
 */
tMAPS_PROTO_PARSED_FRAME * MapsProtoParseFrame(uint8_t *frame, uint16_t size);

// Create MAPS Request Frame
//-----------------------------------------------------------------------------

/** @brief Creates request frame without data. The cmd must be a NULL terminate string.
 *         If greater than 2, only the first 2 bytes are considered.
 *
 *  The commands support this function are:
 *
 *      DE (Barrier status)
 *      EA (Vehicle heights)
 *      FA (Change the barrier to normal operation mode)
 *      MV (Verify if the barrier is operative or is present)
 *      PA (Execute the special adjust command)
 *      AC (Execute the adjust command)
 *      RF (Execute the master reset command)
 *      TT (Execute a barrier test)
 *      CB (Get the status of the vehicle detection loop. Executed by the barrier)
 *
 *  And some Spontaneous commands
 *
 *      FP (Vehicle presence end)
 *      IP (Vehicle presence start)
 *      IR (Vehicle presence start backing back)
 *      RE (Barrier reset) with CF24P & CF150. For CF220 use MapsProtoCreateRERequest
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid CMD argument (Is NULL or Unknown/Unsupported CMD) or num is out of range.
 *
 * @param  num The message number to use. Range 0 to 9.
 * @param  cmd The command to set. Must be a NULL terminate string.. See function description for a list of commands.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEmptyRequest(uint8_t num, const char *cmd);

/** @brief Creates a message for set the COM baud rate to use.
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *
 *  The possible values are:
 *
 *      1: 9600 bps
 *      2: 19200 bps
 *      3: 38400 bps
 *      4: 57600 bps
 *      5: 115200 bps
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in num param.
 *
 * @param  num The message number to use. Range 0 to 9.
 * @param  baud_rate The baud rate to set. If isn´t a valid value then the baud rate is set to 1 (9600 bps)
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateBRRequest(uint8_t num, uint8_t baud_rate);

/** @brief Creates a message to set the config for the max allowable anomalies.
 *
 *  Command that allows selecting the maximum number of disabled sensors to generate the spontaneous message (EM).
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in num param
 *
 * @param  num The message number to use. Range 0 to 9.
 * @param  ncs_down The max sensors down before activate cleaning alarm. Range 01 to MAX BARRIER SENSORS.
 * @param  nds_down The max sensors down before activate degraded alarm. Range 01 to MAX BARRIER SENSORS.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateCARequest(uint8_t num, uint8_t ncs_down, uint8_t nds_down);

/** @brief Creates a message to get the status of specified receiver.
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in some param
 *
 * @param  num       The message number to use. Range 0 to 9.
 * @param  pcell_num The receiver number to retrieve the status. Range 01 to 24
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateERRequest(uint8_t num, uint8_t pcell_num);

/** @brief Creates a message to set the delay time for presence relay.
 *
 *  Command that allows modifying the delay time between the absence of the
 *  presence signal and the fall of the presence relay, in a range between 0 and 99ms.
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in num param
 *
 * @param  num       The message number to use. Range 0 to 9.
 * @param  msec_time The delay time to set. Range 00 to 99 in miliseconds. If out of range then is set to 99.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreatePRRequest(uint8_t num, uint8_t msec_time);

/** @brief Creates a message to set the scanner mode options
 *
 *  Once the scanner command is received, the barrier will continuously send
 *  a character map, with information on the reception status of the 48 emitters
 *  that make up the vertical plane or horizontal plane. Depending of the barrier model.
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *
 *  The availables mode are:
 *
 *      ONLY ON CF-24P and 24 horizontal receivers
 *      A: Only when are diferences
 *      B: Continuously
 *      C: Continuously with presence
 *
 *                         CF-24P                   \           CF220M
 *      D: Continuously                             \     if there is change
 *      E: Only if there is a vehicle present       \     continuously.
 *      H: Continuously (LF)                        \     if there is a change
 *      I: Only if there is vehicle presence (LF)   \     continuously.
 *
 *   For msec_time the minimum time is 5ms when the baud rate is
 *   configured as 115200 bps or 30ms when baudrate is 9600 bps
 *   If the value is greater than 999 the it's set to 999.
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in some param
 *
 * @param  num        The message number to use. Range 0 to 9.
 * @param  mode       The scanner mode to use. See function description for more information.
 * @param  msec_time  The time in miliseconds to use for send the information. Range 000-999.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateSCRequest(uint8_t num, char mode, uint16_t msec_time);

/** @brief Creates a message to set the barrier working mode options
 *
 *  Command that allows selecting the working mode of the barrier:
 *  active / inactive / cleaning, as well as selecting the level of
 *  depth of information that the road controller wants about the
 *  identification of the vehicle.
 *
 *  The available modes are:
 *
 *     0: Cleaning
 *     1: Curtain Inactive
 *     2: Active Curtain
 *     3: Active Curtain with sending message [CF220 only]
 *
 *  The axis detection values are:
 *
 *     IN CF24P IS NOT USED
 *     IN CF150 REPRESENT THE NUMBER OF AXES; 0=OFF 1=ON
 *     IN CF220
 *          0: Axis detection is not sent.
 *          1: Axis detection activated.
 *          2: Axis transition detection and message in the 4 lower sensors.
 *          4: Measurement of the instantaneous speed at the start of the advance.
 *          8: Sends an "EJ" message with the number of axes and speed when detecting a new axis.
 *
 *          If we need the measurement of speed and the detection of axes we will put 5
 *
 *  The axis heights values are:
 *
 *      IN CF24P
 *          0: Msg height in 1st axis.
 *          2: Height detection Msg at the start of the vehicle, at each axle and at the end
 *             of the vehicle; maximum, minimum upper and lower height of the vehicle
 *
 *      IN CF150 height over 1st axis 0=OFF 1=ON
 *
 *      IN CF220
 *          0: The height message is not sent.
 *          1: Height message on the first positive axis activated.
 *          2: Height detection msg at the start of the vehicle, at each axle and at the end
 *             of the vehicle; maximum, minimum upper and lower height of the vehicle
 *
 *  The tow detection is ONLY AVAILABLE IN THE CF-150 & CF-220M MODELS and the values are:
 *
 *      IN CF150 Value R activates trailer detection, value 0 deactivates it.
 *      IN CF220
 *          A: Activation of the trailer message
 *          M: Motorcycle Detection Activation
 *          N: Trailer and motorcycle activation
 *          E: Activation message trailer more axles detected until trailer
 *          T: Activation trailer more axles and motorcycle
 *          0: Deactivation of the trailer and motorcycle message (default).
 *
 *  The barrier direction independent of the position of JUMPER JP1
 *  is ONLY AVAILABLE IN THE CF-220M MODELS and the values are:
 *
 *      P: Receiver column on the left in the direction of vehicle travel.
 *      N: Receiver column on the right in the direction of vehicle travel.
 *
 *  Because this message is different for each barrier you must specify how many
 *  elements of the tMAPS_PROTO_SM_DATA structure are used.
 *
 *  Examples:
 *
 *      IN a CF-24P the elements must be 3 so we only use the first 3 elements of the
 *      tMAPS_PROTO_SM_DATA structure (work_mode, axis_ispeed and axis_height)
 *      for create the message.
 *
 *      IN a CF-150 the elements must be 4 so we only use the first 4 elements of the
 *      tMAPS_PROTO_SM_DATA structure (work_mode, axis_ispeed, axis_height and tow_detection)
 *      for create the message.
 *
 *      IN a CF-220 we use all elements of the tMAPS_PROTO_SM_DATA structure. So elements
 *      parameter must be 5.
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in num/elements params, data is NULL or data have invalid values.
 *
 *
 * @param  num      The message number to use. Range 0 to 9.
 * @param  elements The number of elements to use from tMAPS_PROTO_SM_DATA structure. Range 3 to 5. See description for details.
 * @param  data     The datin SM frame as tMAPS_PROTO_SM_DATA structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateSMRequest(uint8_t num, uint8_t elements, tMAPS_PROTO_SM_DATA *data);

/** @brief Creates a message to set the number of sensors to use for detect a tow.
 *
 *  Command that allows you to select the maximum number of sensors
 *  to determine the detection of a tow hitch.
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-220 BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in some param
 *
 * @param  num         The message number to use. Range 0 to 9.
 * @param  sensors_num The number of sensors for detect a tow. Range 03 to 10
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateSRRequest(uint8_t num, uint8_t sensors_num);

/** @brief Creates a message to set the number of the sensor to use for detect the Height of the first axis
 *
 *  Height Relay: Allows you to assign the number of the sensor that
 *  will activate the contact and the functionality (Height of the first axis or acts as a photocell).
 *
 *  +++ COMMAND ONLY SUPPORTED ON CF-24P BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid range in some param
 *
 * @param  num          The message number to use. Range 0 to 9.
 * @param  mode         Mode to use. 0: Height detection on the first axis. 1: Acts as a photocell.
 * @param  receiver_num Indicating the receiver number, with 01 being the one at the bottom. Range 01 to 24.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateRHRequest(uint8_t num, uint8_t mode, uint8_t receiver_num);

// Create MAPS Spontaneous Request Frame
//-----------------------------------------------------------------------------

/** @brief Creates a barrier adjust (AJ/PA Special) request message.
 *
 *  This function is used for create AJ an PA SPECIAL message. When
 *  type param is 0 the generated message is a PA SPECIAL without frame structure.
 *  i.e. Only a 88 bytes array with a <CR> at the end.
 *
 *  When param type is different from 0 the AJ message with a frame structure is created.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  type The type of frame to create. 0: PA Special other different from 0: AJ.
 * @param  data The response data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateBarrierAdjRequest(uint8_t num, uint8_t type, tMAPS_PROTO_BARRIER_ADJUST *data);

/** @brief Creates a request message for the special scanner mode (SC Special)
 *
 *  +++ REQUEST ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *
 *  When mode in data structure is A, B, C the frame created is only for the BARRIER CF-24P
 *  and the message have the frame structure.
 *
 * When mode in data structure is D, E the message created is for
 * BARRIERS CF-24P and for CF220 without frame structure.
 * i.e. Only a 12 bytes array with a <CR> at the end.
 *
 * When mode in data structure is H, I the message created is for
 * BARRIERS CF-24P and for CF220 without frame structure.
 * i.e. Only a 12 bytes array with a <CR><LF> at the end.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  mode The type of frame to create. 0: PA Special other different from 0: AJ.
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateSCSpecialRequest(uint8_t num, tMAPS_PROTO_SC_SPECIAL *data);

/** @brief Creates a request message for the AP (HEIGHT ABOVE THE FIRST POSITIVE AXIS)
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateAPRequest(uint8_t num, tMAPS_PROTO_AP_DATA *data);

/** @brief Creates a request message for the EJ (NUMBER OF AXES AND SPEED WHEN DETECTING AN AXIS)
 *
 *  +++ REQUEST ONLY SUPPORTED ON CF-220 BARRIERS +++
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEJRequest(uint8_t num, tMAPS_PROTO_EJ_DATA *data);

/** @brief Creates a request message for the EM (BARRIER STATE GENERATION DUE TO MALFUNCTION)
 *
 *  When the rcvr_direction member of the tMAPS_PROTO_EM_DATA structure is 0 the result frame
 *  have the format for the BARRIERS CF-150 or CF24P.
 *
 *  If the rcvr_direction member of the tMAPS_PROTO_EM_DATA structure is P or N the result frame
 *  have the format for the BARRIER CF-220.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEMRequest(uint8_t num, tMAPS_PROTO_EM_DATA *data);

/** @brief Creates a request message for the FA/FR (END PRESENCE VEHICLE MOVING FORWARD/BACKWARD)
 *
 *  +++ REQUEST ONLY SUPPORTED ON CF-220 & CF-150 BARRIERS +++
 *
 *  IF the smbyte member of the tMAPS_PROTO_END_VEHICLE structure is 3 the message
 *  created will have the CF-150 structure with data length 4 bytes.
 *
 *  IF the smbyte member of the tMAPS_PROTO_END_VEHICLE structure is 0 or 1 the message
 *  created will have the CF-220 structure with data length 5 bytes.
 *
 *  IF the smbyte member of the tMAPS_PROTO_END_VEHICLE structure is 2 the message
 *  created will have the CF-220 structure with data length 17 bytes.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values, num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  type The type of command to use in the message. If is 0, then the cmd will be FA otherwise FR
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEndVehicleRequest(uint8_t num, uint8_t type, tMAPS_PROTO_END_VEHICLE *data);

/** @brief Creates a request message for the FX/PX (FAILURE START/FAILURE END)
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values or num param is out of range.
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  type The type of command to use in the message. If is 0, then the cmd will be FX otherwise PX
 * @param  data The request data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateFailureRequest(uint8_t num, uint8_t type, tMAPS_PROTO_FAILURE_DATA *data);

/** @brief Creates a request message for the IA (START PRESENCE VEHICLE MOVING FORWARD)
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-220 & CF-150 BARRIERS +++
 *
 *  The CF-150 barriers not generates data.
 *  The CF-220 barrier may or may not have data. If the instantaneous speed
 *  is activated then we will have data but if is disabled then we will not have data.
 *
 *  If ispeed is 0 then the generated message will be empty (i.e. Without data in the payload)
 *  If ispeed is greater than 99 then ispeed will be 99.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Param num is out of range.
 *
 * @param  num    The message number to use. Range 0 to 9.
 * @param  ispeed The instantaneous speed when is activated or empty if is disabled. Range 00 to 99 Km/h
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateIARequest(uint8_t num, uint8_t ispeed);

/** @brief Creates a request message for the RE (BARRIER RESET)
 *
 *  The CF150 & CF24P barriers not generates data. i.e. The generated raw frame is an empty request.
 *  If you want a message for CF150 or CF24P barriers, one or the 3 params fim_ver, rev_ver & date_ver must be 0.
 *  If you want a message for CF-220 then params fim_ver, rev_ver & date_ver must be set correctly.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Param num is out of range or date_ver param is invalid.
 *
 * @param  num      The message number to use. Range 0 to 9.
 * @param  firm_ver The firmware version 0 for CF150 & CF24P or 1 to 99 for CF220 if greater then set to 99
 * @param  rev_ver  The revision version 0 for CF150 & CF24P or 1 to 99 for CF220 if greater then set to 99
 * @param  date_ver The version date 0 for CF150 & CF24P or ddmmaa for CF220. An invalid date generates EINVAL.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateRERequest(uint8_t num, uint8_t firm_ver, uint8_t rev_ver, uint32_t date_ver);

/** @brief Creates a request message for the RM (TOW DETECTION & TOW DETECTION + AXES)
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-220 & CF-150 BARRIERS +++
 *
 *  The CF-150 barriers not generates data.
 *  The CF-220 barrier may or may not have data. When tow detection + axes
 *  is activated then we will have data with the conditions:
 *
 *  Vehicle in positive direction moving forward:
 *      Number of axles of the vehicle until hitch detection.
 *  Vehicle in negative direction reversing:
 *      Number of trailer axles until hitch detection.
 *
 *  If naxes is 0 then the generated message will be empty (i.e. Without data in the payload)
 *  If naxes is greater than 99 then ispeed will be 99.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Param num is out of range.
 *
 * @param  num   The message number to use. Range 0 to 9.
 * @param  naxes The number of axes. Range 00 to 99
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateRMRequest(uint8_t num, uint8_t naxes);

// Create MAPS Response Frame
//-----------------------------------------------------------------------------

/** @brief Creates a unknown or not executed response message.
 *         The cmd must be a null terminate string with a length of 2.
 *         If the length of the cmd is greater, only the first 2 bytes are considered
 *
 *  This response is executed by the barrier when a command is UNKNOWN
 *  (i.e is not a valid CMD, not supported or when the command cannot be executed).
 *
 *  Actually, this library does not allow the creation of unknown commands, but
 *  this function is intended to respond to commands generated by a data corruption.
 *
 *  The follow list of commands, represents the commands that support not executed
 *  because are the commands that the via controller sends to the barrier.
 *
 *      BR (Change baud rate)
 *      CA (Config max anomalies)
 *      DE (Barrier status)
 *      EA (Vehicle heights)
 *      ER (Receiver status)
 *      FA (Change the barrier to normal operation mode)
 *      MV (Verify if the barrier is operative or is present)
 *      PA (Execute the special adjust command)
 *      AC (Execute the adjust command)
 *      PR (Presence relay fall delay)
 *      RF (Execute the master reset command)
 *      SC (Scanner mode)
 *      SM (Select working mode)
 *      SR (Sensors for taw detection)
 *      TT (Execute a barrier test)
 *      RH (Config contact output)
 *      CB (Get the status of the vehicle detection loop. Executed by the barrier)
 *
 *  This implementation is only for test purposes.
 *  Messages with invalid NUMBER RANGE (out of range 0 to 9) are not responded.
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid CMD argument (Is NULL or Unknown/Unsupported CMD) or num is out of range.
 *
 * @param num The message number to use. Range 0 to 9.
 * @param cmd The command to set. Must be a NULL terminate string. See function description for details.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateUnknownResponse(uint8_t num, const char *cmd);

/** @brief Creates a response frame without data. The cmd must be a NULL terminate string.
 *
 *  This function is for create a response frame without data for
 *  the next response commands:
 *
 *      RSBR (Change Baud rate)
 *      RSCA (Config max anomalies)
 *      RSFA (Change the barrier to normal operation mode)
 *      RSMV (Verify if the barrier is operative or is present)
 *      RSPA (Execute the special adjust command)
 *      RSAC (Execute the adjust command)
 *      RSPR (Presence relay fall delay)
 *      RSRF (Execute the master reset command)
 *      RSSC (Scanner mode)
 *      RSSM (Select working mode)
 *      RSSR (Sensors for taw detection)
 *
 *  And for all Spontaneous Commands:
 *
 *      AJ         (Barrier adjustment)
 *      PA SPECIAL (Barrier adjustment)
 *      SC SPECIAL (Scanner mode)
 *      AP         (Height above first axis)
 *      EJ         (Number of axes and speed)
 *      EM         (Status due to malfunction)
 *      FP         (Vehicle presence end)
 *      FA         (End of vehicle)
 *      FR         (End of vehicle backing back)
 *      FX         (End failure)
 *      IP         (Vehicle presence start)
 *      IA         (Vehicle presence start going forward)
 *      IR         (Vehicle presence start backing back)
 *      PX         (Start failure)
 *      RE         (Barrier reset)
 *      RM         (Taw detection)
 *
 *  The errno values are:
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Invalid CMD argument (Is NULL or Unknown/Unsupported CMD) or num is out of range.
 *
 * @param num The message number to use. Range 0 to 9.
 * @param cmd The command to set. Must be a NULL terminate string. See function description for a list of commands.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEmptyResponse(uint8_t num, const char *cmd);

/** @brief Creates a response message for barrier status
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is invalid. i.e. NULL or not have valid values. Or param num out of range
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The response data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateDEResponse(uint8_t num, tMAPS_PROTO_DE_DATA *data);

/** @brief Creates a response message for state heights
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is NULL or not have valid values. Or param num out of range
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The response data structure. If any value of the structure is greater than 99, then it is set to 99.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateEAResponse(uint8_t num, tMAPS_PROTO_EA_DATA *data);

/** @brief Creates a response message for get the status of receiver
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-220 & CF-24P BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Parameter num out of range
 *
 * @param  num         The message number to use. Range 0 to 9.
 * @param  recv_status The status of the receiver. 0: Not Hidden; 1: Hidden;
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateERResponse(uint8_t num, uint8_t recv_status);

/** @brief Creates a response message for get the barrier test data
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: The data parameter is invalid. i.e. NULL or not have valid values. Or param num out of range
 *
 * @param  num  The message number to use. Range 0 to 9.
 * @param  data The response data structure.
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateTTResponse(uint8_t num, tMAPS_PROTO_TT_DATA *data);

/** @brief Creates a response message for CONTACT OUTPUT CONFIGURATION
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-24P BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Param num is out of range or recvn is out of range.
 *
 * @param  num   The message number to use. Range 0 to 9.
 * @param  wmode The working mode. 0: Height detection on the first axis. 1: Acts as a photocell.
 * @param  recvn Receiver number for height or photocell detection. Range 01 to 24
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateRHResponse(uint8_t num, uint8_t wmode, uint8_t recvn);

/** @brief Creates a response message for the status of the vehicle detection loop
 *
 *  This response is generated by the via controller as response to
 *  the request made it by the barrier about vehicle detection loop.
 *
 *  +++ RESPONSE ONLY SUPPORTED ON CF-150 BARRIERS +++
 *
 *  The errno values are:
 *
 *      ENOMEM: Couldn't allocate memory
 *      EINVAL: Parameter num out of range
 *
 * @param  num        The message number to use. Range 0 to 9.
 * @param  loop_state State of the vehicle detection loop 0: loop disabled. 1: loop enabled
 * @return NULL on error and Errno is set or on sucess a new allocated buffer with the created MAPS frame.
 */
tMAPS_PROTO_RAW_FRAME * MapsProtoCreateCBResponse(uint8_t num, uint8_t loop_state);

//-----------------------------------------------------------------------------
#endif
