#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maps_proto.h"
//-----------------------------------------------------------------------------

#define K_ERROR_REQ_FRAMES 3
#define K_ERROR_BAD_FRAMES 11
#define K_ERROR_MAX_LENGTH 20
//-----------------------------------------------------------------------------

void parse_check_result(const char *test, tMAPS_PROTO_RAW_FRAME *frame)
{
    tMAPS_PROTO_PARSED_FRAME *parsed;

    if (frame)
    {
        if ((parsed = MapsProtoParseFrame(frame->data,frame->size)) == NULL)
            printf("%s test FAILED. Error parsing data. Error: %s\n",test,strerror(errno));
        else
            printf("%s test PASSED\n",test);

        MapsProtoFreeParsedFrame(parsed);
    }
    else
        printf("%s test FAILED. Error creating message. Error: %s\n",test,strerror(errno));

    MapsProtoFreeRawFrame(frame);
}
//-----------------------------------------------------------------------------

void CreateAndParseRequest()
{
    tMAPS_PROTO_RAW_FRAME *frame;
    tMAPS_PROTO_BARRIER_ADJUST badj;

    printf("\n#### REQUEST TESTS ####\n");
    memset(badj.rcv_map3,0x45,K_MAPS_PROTO_RECEIVE_GROUP3);
    memset(badj.rcv_map8,0x46,K_MAPS_PROTO_RECEIVE_GROUP8);

    frame = MapsProtoCreateBRRequest(0,3);                                                                                      parse_check_result("BR",frame);
    frame = MapsProtoCreateCARequest(1,1,2);                                                                                    parse_check_result("CA",frame);
    frame = MapsProtoCreateEmptyRequest(2,"DE");                                                                                parse_check_result("DE",frame);
    frame = MapsProtoCreateEmptyRequest(3,"EA");                                                                                parse_check_result("EA",frame);
    frame = MapsProtoCreateERRequest(4,23);                                                                                     parse_check_result("ER",frame);
    frame = MapsProtoCreateEmptyRequest(5,"FA");                                                                                parse_check_result("FA",frame);
    frame = MapsProtoCreateEmptyRequest(6,"MV");                                                                                parse_check_result("MV",frame);
    frame = MapsProtoCreateEmptyRequest(7,"PA");                                                                                parse_check_result("PA",frame);
    frame = MapsProtoCreateEmptyRequest(8,"AC");                                                                                parse_check_result("AC",frame);
    frame = MapsProtoCreatePRRequest(9,99);                                                                                     parse_check_result("PR",frame);
    frame = MapsProtoCreateEmptyRequest(0,"RF");                                                                                parse_check_result("RF",frame);
    frame = MapsProtoCreateSCRequest(1,'A',999);                                                                                parse_check_result("SC",frame);
    frame = MapsProtoCreateSCRequest(1,'H',999);                                                                                parse_check_result("SC",frame);
    frame = MapsProtoCreateSMRequest(2,3,(tMAPS_PROTO_SM_DATA*)"\x02\x00\x02\x00\x00");                                         parse_check_result("SM",frame);
    frame = MapsProtoCreateSMRequest(2,4,(tMAPS_PROTO_SM_DATA*)"\x01\x01\x01\x52\x00");                                         parse_check_result("SM",frame);
    frame = MapsProtoCreateSMRequest(2,5,(tMAPS_PROTO_SM_DATA*)"\x03\x08\x02\x54\x4E");                                         parse_check_result("SM",frame);
    frame = MapsProtoCreateSRRequest(3,4);                                                                                      parse_check_result("SR",frame);
    frame = MapsProtoCreateEmptyRequest(4,"TT");                                                                                parse_check_result("TT",frame);
    frame = MapsProtoCreateRHRequest(5,1,20);                                                                                   parse_check_result("RH",frame);
    frame = MapsProtoCreateEmptyRequest(6,"CB");                                                                                parse_check_result("CB",frame);

    // Spontaneous Messages
    printf("\n#### REQUEST SPONTANEOUS TESTS ####\n");
    frame = MapsProtoCreateBarrierAdjRequest(7,1,&badj);                                                                        parse_check_result("AJ" ,frame);
    frame = MapsProtoCreateBarrierAdjRequest(8,0,&badj);                                                                        parse_check_result("PAS",frame);
    frame = MapsProtoCreateSCSpecialRequest(9,(tMAPS_PROTO_SC_SPECIAL*)"\x41\x00\x46\x46\x46\x46\x46\x46\x00\x00\x00\x00\x00"); parse_check_result("SCS",frame);
    frame = MapsProtoCreateSCSpecialRequest(0,(tMAPS_PROTO_SC_SPECIAL*)"\x44\x46\x46\x46\x46\x46\x46\x46\x45\x45\x45\x45\x45"); parse_check_result("SCS",frame);
    frame = MapsProtoCreateSCSpecialRequest(1,(tMAPS_PROTO_SC_SPECIAL*)"\x48\x46\x46\x46\x46\x46\x46\x46\x45\x45\x45\x45\x45"); parse_check_result("SCS",frame);
    frame = MapsProtoCreateAPRequest(0,(tMAPS_PROTO_AP_DATA*)"\x00\x0C\x00\x00\x0F\x0B\x14\x28");                               parse_check_result("AP" ,frame);
    frame = MapsProtoCreateAPRequest(0,(tMAPS_PROTO_AP_DATA*)"\x02\x0C\x00\x00\x0F\x0B\x14\x28");                               parse_check_result("AP" ,frame);
    frame = MapsProtoCreateEJRequest(1,(tMAPS_PROTO_EJ_DATA *)"\x09\x03\x88");                                                  parse_check_result("EJ" ,frame);
    frame = MapsProtoCreateEMRequest(2,(tMAPS_PROTO_EM_DATA *)"\x02\x00\x02\x00\x01\x02\x1F\x00\x00");                          parse_check_result("EM" ,frame);
    frame = MapsProtoCreateEMRequest(2,(tMAPS_PROTO_EM_DATA *)"\x03\x08\x02\x4D\x01\x02\x1F\x50\x00");                          parse_check_result("EM" ,frame);
    frame = MapsProtoCreateEmptyRequest(3,"FP");                                                                                parse_check_result("FP" ,frame);
    frame = MapsProtoCreateEndVehicleRequest(4,0,(tMAPS_PROTO_END_VEHICLE *)"\x01\x43\x09\x09\x63\x00\x00\x00\x00\x00");        parse_check_result("FAS",frame);
    frame = MapsProtoCreateEndVehicleRequest(4,0,(tMAPS_PROTO_END_VEHICLE *)"\x02\x43\x09\x09\x00\x00\x00\x00\x00\x00");        parse_check_result("FAS",frame);
    frame = MapsProtoCreateEndVehicleRequest(4,0,(tMAPS_PROTO_END_VEHICLE *)"\x03\x43\x09\x09\x00\x00\x63\x00\x00\x00");        parse_check_result("FAS",frame);
    frame = MapsProtoCreateEndVehicleRequest(5,1,(tMAPS_PROTO_END_VEHICLE *)"\x01\x43\x09\x09\x00\x00\x00\x00\x63\x00");        parse_check_result("FR" ,frame);
    frame = MapsProtoCreateEndVehicleRequest(5,1,(tMAPS_PROTO_END_VEHICLE *)"\x02\x43\x09\x09\x00\x00\x00\x00\x00\x00");        parse_check_result("FR" ,frame);
    frame = MapsProtoCreateEndVehicleRequest(5,1,(tMAPS_PROTO_END_VEHICLE *)"\x03\x43\x09\x09\x00\x63\x00\x00\x00\x00");        parse_check_result("FR" ,frame);
    frame = MapsProtoCreateFailureRequest(6,0,(tMAPS_PROTO_FAILURE_DATA *)"\x52\x06\x04");                                      parse_check_result("FX" ,frame);
    frame = MapsProtoCreateEmptyRequest(7,"IP");                                                                                parse_check_result("IP" ,frame);
    frame = MapsProtoCreateIARequest(8,0);                                                                                      parse_check_result("IA" ,frame);
    frame = MapsProtoCreateIARequest(8,9);                                                                                      parse_check_result("IA" ,frame);
    frame = MapsProtoCreateEmptyRequest(9,"IR");                                                                                parse_check_result("IR" ,frame);
    frame = MapsProtoCreateFailureRequest(0,1,(tMAPS_PROTO_FAILURE_DATA *)"\x45\x08\x08");                                      parse_check_result("PX" ,frame);
    frame = MapsProtoCreateEmptyRequest(1,"RE");                                                                                parse_check_result("RE" ,frame);
    frame = MapsProtoCreateRMRequest(2,0);                                                                                      parse_check_result("RM" ,frame);
    frame = MapsProtoCreateRMRequest(2,9);                                                                                      parse_check_result("RM" ,frame);
}
//-----------------------------------------------------------------------------

void CreateAndParseResponse()
{
    tMAPS_PROTO_RAW_FRAME *frame;
    tMAPS_PROTO_TT_DATA ttdata = { .mvar = 'M', .rvar = 'R', };

    printf("\n#### RESPONSE TESTS ####\n");
    memset(ttdata.e_map,0x37,K_MAPS_PROTO_EMITTERS_MAP_SIZE);
    memset(ttdata.r_map,0x35,K_MAPS_PROTO_RECEIVERS_MAP_SIZE);

    frame = MapsProtoCreateUnknownResponse(0,"XX");                                                                             parse_check_result("XX" ,frame);
    frame = MapsProtoCreateEmptyResponse(1,"BR");                                                                               parse_check_result("BR" ,frame);
    frame = MapsProtoCreateDEResponse(2,(tMAPS_PROTO_DE_DATA*)"\x00\x00\x02\x00\x01\x02\x0B\x50\x03");                          parse_check_result("DE" ,frame);
    frame = MapsProtoCreateEAResponse(3,(tMAPS_PROTO_EA_DATA*)"\x0F\x16\x50\x63");                                              parse_check_result("EA" ,frame);
    frame = MapsProtoCreateERResponse(4,0);                                                                                     parse_check_result("ER" ,frame);
    frame = MapsProtoCreateTTResponse(5,&ttdata);                                                                               parse_check_result("TT" ,frame);
    frame = MapsProtoCreateRHResponse(6,0,20);                                                                                  parse_check_result("RH" ,frame);
    frame = MapsProtoCreateCBResponse(7,0);                                                                                     parse_check_result("CB" ,frame);
}
//-----------------------------------------------------------------------------

void CreateAndParseErrors()
{
    tMAPS_PROTO_RAW_FRAME    *frames[K_ERROR_REQ_FRAMES];
    tMAPS_PROTO_PARSED_FRAME *parsed[K_ERROR_BAD_FRAMES];

    // Creation errors. Applies the same way for requests as responses.
    frames[0] = MapsProtoCreateBRRequest(10,3);      // Bad number
    frames[1] = MapsProtoCreateEmptyRequest(2,"XX"); // Bad cmd
    frames[2] = MapsProtoCreateSCSpecialRequest(9,(tMAPS_PROTO_SC_SPECIAL*)"\x00\x00\x46\x46\x46\x46\x46\x46\x00\x00\x00\x00\x00"); // Bad data (mode)

    printf("\n#### ERROR REQ TESTS ####\n");
    for (uint8_t i = 0; i < K_ERROR_REQ_FRAMES; i++)
    {
        if (frames[i])
            printf("ERROR REQ TEST # %u FAILED\n",i);
        else
            printf("ERROR REQ TEST # %u PASSED\n",i);

        MapsProtoFreeRawFrame(frames[i]);
    }

    // Bad frames. The parse should fail.
    uint8_t bad_frames[K_ERROR_BAD_FRAMES][K_ERROR_MAX_LENGTH] =
    {
        {0x06,0x00,0x00,0x00,0x00,0x00,0x00},                                                                  // Bad lenght.   Frame to short
        {0x08,0x00,0x01,0x42,0x52,0x31,0x30,0x31,0x0A},                                                        // Bad frame.    Not start and end bytes.
        {0x08,0x01,0x0A,0x42,0x52,0x31,0x30,0x31,0x0D},                                                        // Bad request.  Invalid num
        {0x08,0x01,0x09,0x58,0x58,0x31,0x30,0x31,0x0D},                                                        // Bad request.  Unknown cmd
        {0x08,0x01,0x09,0x42,0x52,0x31,0x30,0x31,0x0D},                                                        // Bad request.  Bad Checksum
        {0x0A,0x01,0x09,0x52,0x53,0x58,0x58,0x48,0x30,0x31,0x0D},                                              // Bad response. Unknown cmd
        {0x11,0x01,0x05,0x41,0x50,0x4E,0x00,0x00,0x00,0x00,0x15,0x00,0x08,0x00,0x08,0x30,0x31,0x0D},           // Bad request.  Invalid data
        {0x12,0x01,0x05,0x41,0x50,0x4E,0x00,0x00,0x00,0x00,0x15,0x00,0x08,0x00,0x08,0x00,0x30,0x31,0x0D},      // Bad request.  Invalid data length
        {0x11,0x01,0x05,0x45,0x4D,0x04,0x00,0x00,0x00,0x00,0x15,0x00,0x08,0x00,0x08,0x30,0x31,0x0D},           // Bad request.  Invalid data
        {0x12,0x01,0x05,0x52,0x53,0x44,0x45,0x30,0x30,0x30,0x30,0x31,0x32,0x31,0x35,0x30,0x31,0x31,0x0D},      // Bad response. Invalid data length
        {0x13,0x01,0x05,0x52,0x53,0x44,0x45,0x34,0x30,0x30,0x30,0x31,0x32,0x31,0x35,0x30,0x30,0x31,0x31,0x0D}, // Bad response. Invalid data
    };

    printf("\n#### ERROR PARSE TESTS ####\n");
    for (uint8_t i = 0; i < K_ERROR_BAD_FRAMES; i++)
    {
         parsed[i] = MapsProtoParseFrame(&bad_frames[i][1],bad_frames[i][0]);
         if (parsed[i])
             printf("ERROR PARSE TEST # %u FAILED\n",i);
         else
             printf("ERROR PARSE TEST # %u PASSED\n",i);

         MapsProtoFreeParsedFrame(parsed[i]);
    }
}
//-----------------------------------------------------------------------------

tMAPS_PROTO_RAW_FRAME * CreateRequest(char type)
{
    switch (type)
    {
        case 'a': // BR (BaudRate)
                return MapsProtoCreateBRRequest(0,1);
        break;
        case 'b': // CA (Max Anomalies)
                return MapsProtoCreateCARequest(1,1,2);
        break;
        case 'c': // DE (Barrier Status)
                return MapsProtoCreateEmptyRequest(2,"DE");
        break;
        case 'd': // EA (Hights Status)
                return MapsProtoCreateEmptyRequest(3,"EA");
        break;
        case 'e': // ER (Receptor Status)
                return MapsProtoCreateERRequest(4,23);
        break;
        case 'f': // FA (End Adjust)
                return MapsProtoCreateEmptyRequest(5,"FA");
        break;
        case 'g': // MV (Operative Barrier)
                return MapsProtoCreateEmptyRequest(6,"MV");
        break;
        case 'h': // PA (Barrier Adjust)
                return MapsProtoCreateEmptyRequest(7,"PA");
        break;
        case 'i': // AC (Barrier Adjust)
                return MapsProtoCreateEmptyRequest(8,"AC");
        break;
        case 'j': // PR (Relay delay)
                return MapsProtoCreatePRRequest(9,99);
        break;
        case 'k': // RF (Master Reset)
                return MapsProtoCreateEmptyRequest(0,"RF");
        break;
        case 'l': // SC (Scan Mode. Modes D,E)
                return MapsProtoCreateSCRequest(1,'D',999);
        break;
        case 'm': // SC (Scan Mode. Modes H,I)
                return MapsProtoCreateSCRequest(2,'H',999);
        break;
        case 'n': // SM (Working Mode. Inactive)
                return MapsProtoCreateSMRequest(3,5,(tMAPS_PROTO_SM_DATA*)"\x01\x01\x01\x00\x50");
        break;
        case 'o': // SM (Working Mode. All hights enabled)
                return MapsProtoCreateSMRequest(4,5,(tMAPS_PROTO_SM_DATA*)"\x02\x04\x02\x52\x50");
        break;
        case 'p': // SM (Working Mode. Enable send Msg and all hights.)
                return MapsProtoCreateSMRequest(5,5,(tMAPS_PROTO_SM_DATA*)"\x03\x05\x02\x54\x4E");
        break;
        case 'q': // SR (Num sensors for tow detection)
                return MapsProtoCreateSRRequest(6,4);
        break;
        case 'r': // TT (Test Barrier)
                return MapsProtoCreateEmptyRequest(7,"TT");
        break;
    }

    return NULL;
}
//-----------------------------------------------------------------------------

int main()
{
    //uint8_t data[] = {0x01,0x30,0x52,0x45,0x2f,0x33,0x32,0x43,0x46,0x2d,0x32,0x32,0x30,0x4d,0x2f,0x56,0x2d,0x33,0x30,0x2f,0x52,0x2d,0x30,0x31,0x2f,0x44,0x2d,0x30,0x33,0x2d,0x30,0x32,0x2d,0x32,0x31,0x2f,0x33,0x31,0x0d};
    //uint8_t data[] = {0x01,0x37,0x52,0x53,0x54,0x54,0x4D,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x52,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x32,0x39,0x0D};
    //uint8_t data[] = {0x01,0x32,0x52,0x53,0x44,0x45,0x31,0x30,0x30,0x30,0x31,0x31,0x33,0x30,0x50,0x34,0x35,0x34,0x0D};
    //uint8_t data[] = {0x01,0x33,0x52,0x53,0x45,0x41,0x30,0x31,0x30,0x31,0x31,0x35,0x30,0x36,0x33,0x34,0x0D};
    uint8_t data[] = {0x01,0x36,0x52,0x53,0x53,0x52,0x30,0x34,0x33,0x32,0x0D};
    tMAPS_PROTO_PARSED_FRAME *parsed = MapsProtoParseFrame(data,sizeof(data));

    if (parsed == NULL)
        printf("Error: %s\n",strerror(errno));
    MapsProtoFreeParsedFrame(parsed);

    CreateAndParseErrors();
    CreateAndParseRequest();
    CreateAndParseResponse();

    return 0;
}
//-----------------------------------------------------------------------------
