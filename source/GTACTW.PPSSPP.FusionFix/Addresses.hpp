#pragma once
#include "../Shared/Console/NativeProfile.hpp"
#include "../Shared/Console/PSP.hpp"
namespace ctw {
namespace profile_data {
inline constexpr uint32_t radarWords[]={0x27BDFEF0u,0xAFB000E0u,0x00808025u,0xAFB100E4u,0xAFB200E8u,0xAFB300ECu,0xAFB400F0u,0xAFB500F4u,0xAFB600F8u,0xAFB700FCu};
inline constexpr uint8_t radarMasks[]={0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words0[]={0x8FB00004u,0x8FBF0008u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x3C040000u,0x8C840000u,0x3C050000u,0x00042080u,0x24A50000u,0x00852021u,0xAFBF0000u};
inline constexpr uint8_t masks0[]={0,0,0,0,0,1,1,1,0,1,0,0};
inline constexpr uint32_t words1[]={0x00A42023u,0x00042C00u,0x27B20030u,0x00052C03u,0x0C000000u,0x02402025u,0x27A40050u,0x02402825u,0x0C000000u,0x02203025u,0x87A40050u,0x87A50052u};
inline constexpr uint8_t masks1[]={0,0,0,0,2,0,0,0,2,0,0,0};
inline constexpr uint32_t words2[]={0x8CC70000u,0x0087402Au,0x11000004u,0x00000000u,0xA2050008u,0x10000007u,0x92050008u,0x00872023u,0x24A50001u,0x28A70005u,0x14E0FFF5u,0x24C60004u};
inline constexpr uint8_t masks2[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words3[]={0x8FB10004u,0x8FBF0008u,0x03E00008u,0x27BD0010u,0x27BDFFE0u,0xAFB10014u,0x3C110000u,0xAFB00010u,0x00808025u,0x26310000u,0xAFB20018u,0xAFBF001Cu};
inline constexpr uint8_t masks3[]={0,0,0,0,0,0,1,0,0,1,0,0};
inline constexpr uint32_t words4[]={0x8FB20018u,0x8FBF001Cu,0x03E00008u,0x27BD0020u,0x27BDFDF0u,0x3C070000u,0x90E70000u,0xAFB001ECu,0xAFB101F0u,0x00808025u,0x00A08825u,0xAFB201F4u};
inline constexpr uint8_t masks4[]={0,0,0,0,0,1,1,0,0,0,0,0};
inline constexpr uint32_t words5[]={0x00000000u,0x02001025u,0x8FB00000u,0x8FBF0004u,0x03E00008u,0x27BD0010u,0x27BDFFE0u,0xAFB00000u,0x00A08025u,0xAFB10004u,0xAFB20008u,0xAFB3000Cu};
inline constexpr uint8_t masks5[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words6[]={0x8FB3000Cu,0x8FBF0010u,0x03E00008u,0x27BD0020u,0x27BDFFE0u,0xAFB00000u,0x00A08025u,0xAFB10004u,0xAFB20008u,0xAFB3000Cu,0xAFBF0010u,0x0C000000u};
inline constexpr uint8_t masks6[]={0,0,0,0,0,0,0,0,0,0,0,2};
inline constexpr uint32_t words7[]={0x8FB3000Cu,0x8FBF0010u,0x03E00008u,0x27BD0020u,0x27BDFFF0u,0xAFB00000u,0x00A08025u,0xAFB10004u,0xAFBF0008u,0x0C000000u,0x00808825u,0x1440000Bu};
inline constexpr uint8_t masks7[]={0,0,0,0,0,0,0,0,0,2,0,0};
inline constexpr uint32_t words8[]={0x8C840074u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x00853021u,0x80C60078u,0xAFB10004u,0xAFB20008u,0x34120000u,0x2407FFFFu,0x00808825u};
inline constexpr uint8_t masks8[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words9[]={0x8FB0002Cu,0x8FBF0030u,0x03E00008u,0x27BD0040u,0x27BDFFE0u,0xAFB00000u,0xAFB10004u,0xAFB3000Cu,0x00A08025u,0x00808825u,0x30D300FFu,0xAFB20008u};
inline constexpr uint8_t masks9[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words10[]={0x8FA400ACu,0x8FA400ACu,0x8FA500B0u,0xAEC40100u,0x8FA400B4u,0xAEC50104u,0xAEC40108u,0x34041000u,0x8FA60074u,0x34056000u,0xAFA400E0u,0x00C5202Au};
inline constexpr uint8_t masks10[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words11[]={0xAFB20008u,0xAFB40010u,0xAFB50014u,0xAFBF0018u,0x0C000000u,0x00000000u,0x34150000u,0x02B0202Au,0x1080001Eu,0x3C140000u,0x3C120000u,0x26940000u};
inline constexpr uint8_t masks11[]={0,0,0,0,2,0,0,0,0,1,1,1};
inline constexpr uint32_t words12[]={0x00000000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFF30u,0xAFB100ACu,0x00A08825u,0xAFB000A8u,0xAFB200B0u,0xAFB300B4u,0x8E330000u,0x00808025u};
inline constexpr uint8_t masks12[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words13[]={0x8FBE00C8u,0x8FBF00CCu,0x03E00008u,0x27BD00D0u,0x34040008u,0x3C050000u,0xACA40000u,0x34060009u,0x24A40000u,0x03E00008u,0xAC860004u,0x27BDFFF0u};
inline constexpr uint8_t masks13[]={0,0,0,0,0,1,1,0,1,0,0,0};
inline constexpr uint32_t words14[]={0x8FBE0020u,0x8FBF0024u,0x03E00008u,0x27BD0030u,0x27BDFFE0u,0xAFB00000u,0x00808025u,0x8E04002Cu,0xAFB10004u,0x00A08825u,0x8C850000u,0xAFB20008u};
inline constexpr uint8_t masks14[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words15[]={0x8FB3000Cu,0x8FBF0010u,0x03E00008u,0x27BD0020u,0x27BDFF60u,0xAFB00070u,0x01008025u,0xAFB60088u,0xAFB7008Cu,0x00E0B825u,0xA2000000u,0x0080B025u};
inline constexpr uint8_t masks15[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words16[]={0x25080000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x8C860000u,0x3C073E00u,0xACC70000u,0x8C860000u,0x3C073F00u,0x24C60004u,0xAC860000u,0xC4AC0000u};
inline constexpr uint8_t masks16[]={1,0,0,0,1,0,0,1,0,0,1,1};
inline constexpr uint32_t words18[]={0x4600A306u,0x10000001u,0xC64C0C1Cu,0x8FA4004Cu,0x0C000000u,0x02602825u,0x12200010u,0x00000000u,0x8E041EC0u,0x3C052300u,0xAC9E0000u,0x8E041EC0u};
inline constexpr uint8_t masks18[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t words19[]={0x34040002u,0xAE0400B8u,0x1000001Bu,0x00000000u,0x8E0400A0u,0x24840000u,0x84850000u,0x8C860000u,0x00C0F809u,0x02052021u,0x10000013u,0x00000000u};
inline constexpr uint8_t masks19[]={0,0,0,0,0,1,1,1,0,0,0,0};
inline constexpr uint32_t words20[]={0x00C0F809u,0x02052021u,0x10000013u,0x00000000u,0x8E0400A0u,0x24840000u,0x84850000u,0x8C860000u,0x00C0F809u,0x02052021u,0x1000000Bu,0x00000000u};
inline constexpr uint8_t masks20[]={0,0,0,0,0,1,1,1,0,0,0,0};
inline constexpr uint32_t words21[]={0x03E00008u,0x27BD0010u,0x03E00008u,0x3402005Au,0x27BDFF70u,0xAFB00070u,0xAFB10074u,0xAFB20078u,0xAFB3007Cu,0xAFB40080u,0xAFBF0084u,0x0C000000u};
inline constexpr uint8_t masks21[]={0,0,0,0,0,0,0,0,0,0,0,2};
inline constexpr uint32_t words22[]={0x8FB40080u,0x8FBF0084u,0x03E00008u,0x27BD0090u,0x27BDFEE0u,0xAFB40100u,0x3C140000u,0xAFB300FCu,0x00809825u,0x26940000u,0xAFB000F0u,0xAFB100F4u};
inline constexpr uint8_t masks22[]={0,0,0,0,0,0,1,0,0,1,0,0};
inline constexpr uint32_t words23[]={0x8FB10004u,0x8FBF0008u,0x03E00008u,0x27BD0010u,0x27BDFFE0u,0xAFB10004u,0xAFB20008u,0x00808825u,0x00069400u,0x92260F1Au,0xAFB3000Cu,0x0006302Bu};
inline constexpr uint8_t masks23[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words24[]={0x8FB40010u,0x8FBF0014u,0x03E00008u,0x27BD0020u,0x27BDFFF0u,0x8C850000u,0x03A02025u,0xAFBF0004u,0x0C000000u,0xAFA50000u,0x24420080u,0x8FBF0004u};
inline constexpr uint8_t masks24[]={0,0,0,0,0,1,0,0,2,0,0,0};
inline constexpr uint32_t words25[]={0xACA00000u,0x90840047u,0x03E00008u,0xACC40000u,0x27BDFFF0u,0xAFB00000u,0x00A08025u,0xAE000028u,0xAFB10004u,0xAE00002Cu,0x00808825u,0xAFBF0008u};
inline constexpr uint8_t masks25[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words26[]={0x8FB10004u,0x8FBF0008u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0xAFBF0000u,0x0C000000u,0x30A500FFu,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFE0u};
inline constexpr uint8_t masks26[]={0,0,0,0,0,0,2,0,0,0,0,0};
inline constexpr uint32_t words27[]={0x8FBE015Cu,0x8FBF0160u,0x03E00008u,0x27BD0170u,0x27BDFCE0u,0xAFB70310u,0x00A0B825u,0xAEE00028u,0xAEE0002Cu,0xAFBE0314u,0x0080F025u,0xAEE00030u};
inline constexpr uint8_t masks27[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words28[]={0x8FBE0314u,0x8FBF0318u,0x03E00008u,0x27BD0320u,0x27BDFD80u,0xAFB00268u,0x00808025u,0x8E070030u,0x00A02025u,0x8E050034u,0xAFA70018u,0x8E070038u};
inline constexpr uint8_t masks28[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words29[]={0x8FB3000Cu,0x8FBF0010u,0x03E00008u,0x27BD0020u,0x27BDFE00u,0xAFB001D4u,0xAFB101D8u,0x00808025u,0x00A08825u,0xAFB201DCu,0x8E050EBCu,0x00C09025u};
inline constexpr uint8_t masks29[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words30[]={0x8FBE01F4u,0x8FBF01F8u,0x03E00008u,0x27BD0200u,0x27BDFEA0u,0xAFB00138u,0xAFB1013Cu,0xAFB20140u,0xAFB30144u,0xAFB40148u,0xAFB5014Cu,0xAFB60150u};
inline constexpr uint8_t masks30[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words31[]={0x8FB20008u,0x8FBF000Cu,0x03E00008u,0x27BD0010u,0x27BDFF30u,0xAFB300B0u,0x8CB30000u,0x8C870000u,0xAFB000A4u,0x8CE70000u,0xAFB100A8u,0xAFB400B4u};
inline constexpr uint8_t masks31[]={0,0,0,0,0,0,0,1,0,0,0,0};
inline constexpr uint32_t words32[]={0x8FB700C0u,0x8FBF00C4u,0x03E00008u,0x27BD00D0u,0x34040009u,0x3C050000u,0xACA40000u,0x34060011u,0x24A40000u,0x03E00008u,0xAC860004u,0x3C020000u};
inline constexpr uint8_t masks32[]={0,0,0,0,0,1,1,0,1,0,0,1};
inline constexpr uint32_t words33[]={0x86C40F16u,0x30840001u,0x10800004u,0x00000000u,0x02C02025u,0x0C000000u,0x02A02825u,0x92C40F1Cu,0x0004202Bu,0x308400FFu,0x10800058u,0x8E160008u};
inline constexpr uint8_t masks33[]={0,0,0,0,0,2,0,0,0,0,0,0};
inline constexpr uint32_t words34[]={0x8FBE0098u,0x8FBF009Cu,0x03E00008u,0x27BD00A0u,0x27BDFED0u,0xAFB20114u,0x8CB20000u,0xAFB0010Cu,0x00808025u,0xAFB10110u,0xAFB30118u,0xAFB4011Cu};
inline constexpr uint8_t masks34[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words35[]={0x8FB4011Cu,0x8FBF0120u,0x03E00008u,0x27BD0130u,0x27BDFFE0u,0xAFB20008u,0x8CB20000u,0xAFB00000u,0x00A08025u,0xAFB10004u,0xAFB3000Cu,0xAFB40010u};
inline constexpr uint8_t masks35[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words36[]={0x8FB50014u,0x8FBF0018u,0x03E00008u,0x27BD0020u,0x34040009u,0x3C050000u,0xACA40000u,0x34060012u,0x24A40000u,0x03E00008u,0xAC860004u,0x3C020000u};
inline constexpr uint8_t masks36[]={0,0,0,0,0,1,1,0,1,0,0,1};
inline constexpr uint32_t words37[]={0x8FB3000Cu,0x8FBF0010u,0x03E00008u,0x27BD0020u,0x27BDFFE0u,0xAFB10004u,0x00808825u,0xAFB00000u,0x00A08025u,0x00C02025u,0xAFB20008u,0xAFB3000Cu};
inline constexpr uint8_t masks37[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words38[]={0x8FB40010u,0x8FBF0014u,0x03E00008u,0x27BD0020u,0x27BDFFD0u,0xAFB0000Cu,0xAFB30018u,0x00E09825u,0x00808025u,0xAFB10010u,0xAFB20014u,0x00A08825u};
inline constexpr uint8_t masks38[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words39[]={0x8FB00000u,0x8FBF0004u,0x03E00008u,0x27BD0010u,0x27BDFFA0u,0xAFB00050u,0xAFB10054u,0x00A08825u,0x00808025u,0x02202025u,0xAFB20058u,0xAFBF005Cu};
inline constexpr uint8_t masks39[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words40[]={0x8FB20058u,0x8FBF005Cu,0x03E00008u,0x27BD0060u,0x27BDFFA0u,0xAFB10040u,0x00A08825u,0xAFB0003Cu,0xAFB20044u,0xAFB30048u,0x8E330000u,0x00C09025u};
inline constexpr uint8_t masks40[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words41[]={0x8FB60054u,0x8FBF0058u,0x03E00008u,0x27BD0060u,0x27BDFF60u,0xAFB3007Cu,0x8CD30000u,0x310800FFu,0xAFA8006Cu,0xAFB00070u,0x92700F26u,0xAFB20078u};
inline constexpr uint8_t masks41[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words42[]={0x8FBE0090u,0x8FBF0094u,0x03E00008u,0x27BD00A0u,0x27BDFF70u,0xAFB3006Cu,0xAFB50074u,0x00809825u,0x8CB50000u,0xAFBE0080u,0x8E7E0028u,0x8E670030u};
inline constexpr uint8_t masks42[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words43[]={0x8FBE0080u,0x8FBF0084u,0x03E00008u,0x27BD0090u,0x27BDFEE0u,0xAFB00100u,0xAFB40110u,0xAFB50114u,0x00A0A825u,0x00E08025u,0x0080A025u,0xAFB10104u};
inline constexpr uint8_t masks43[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words44[]={0x8FB600C0u,0x8FBF00C4u,0x03E00008u,0x27BD00D0u,0x27BDFFD0u,0xAFB50014u,0xAFB60018u,0x8CB50000u,0x30F600FFu,0x8C870008u,0xAFB00000u,0xAFB10004u};
inline constexpr uint8_t masks44[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words45[]={0x8FBE0020u,0x8FBF0024u,0x03E00008u,0x27BD0030u,0x27BDFF90u,0xAFB1004Cu,0x00A08825u,0xAFB00048u,0xAFB20050u,0xAFB30054u,0x8E330000u,0x00808025u};
inline constexpr uint8_t masks45[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words46[]={0x3C040000u,0x24820000u,0x03E00008u,0x24420000u,0x3C040000u,0x24820000u,0x03E00008u,0x24420000u,0x3C040000u,0x24820000u,0x03E00008u,0x24420000u};
inline constexpr uint8_t masks46[]={1,1,0,1,1,1,0,1,1,1,0,1};
inline constexpr uint32_t words47[]={0x8C840000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x8C840000u,0x3C050000u,0x00042080u,0x24A50000u,0x00852021u,0xAFBF0000u,0x0C000000u};
inline constexpr uint8_t masks47[]={0,0,0,0,0,0,1,0,1,0,0,2};
inline constexpr uint32_t words48[]={0x34050018u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x8C840000u,0x3C050000u,0x00042080u,0x24A50000u,0x00852021u,0xAFBF0000u,0x0C000000u};
inline constexpr uint8_t masks48[]={0,0,0,0,0,0,1,0,1,0,0,2};
inline constexpr uint32_t words49[]={0x34050019u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x8C840000u,0x3C050000u,0x00042080u,0x24A50000u,0x00852021u,0xAFBF0000u,0x0C000000u};
inline constexpr uint8_t masks49[]={0,0,0,0,0,0,1,0,1,0,0,2};
inline constexpr uint32_t words50[]={0x90820000u,0x3C040000u,0x03E00008u,0x90820000u,0x27BDFFF0u,0xAFBF0000u,0x0C000000u,0x00000000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x3C040000u};
inline constexpr uint8_t masks50[]={1,1,0,1,0,0,2,0,0,0,0,1};
inline constexpr uint32_t words51[]={0x00000000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0xAFB10004u,0x30A500FFu,0x2411FFFFu,0xAFB00000u,0xAFB20008u,0xAFBF000Cu,0x14A00004u};
inline constexpr uint8_t masks51[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words52[]={0x8FB20008u,0x8FBF000Cu,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0xAFB00000u,0xAFBF0004u,0x0C000000u,0x00A08025u,0x8E040000u,0x24840000u,0x2C85000Bu};
inline constexpr uint8_t masks52[]={0,0,0,0,0,0,0,2,0,0,1,0};
inline constexpr uint32_t words53[]={0x34050002u,0xA0850000u,0x03E00008u,0x00000000u,0x27BDFF80u,0x30A500FFu,0x3C060000u,0xAFB00060u,0x00052880u,0x24C60000u,0x00808025u,0x00A62021u};
inline constexpr uint8_t masks53[]={0,0,0,0,0,0,1,0,0,1,0,0};
inline constexpr uint32_t words54[]={0x8FB60078u,0x8FBF007Cu,0x03E00008u,0x27BD0080u,0x27BDFFF0u,0xAFB10004u,0x00808825u,0x30E400FFu,0x34070001u,0xA227004Au,0xAFB00000u,0xAFB20008u};
inline constexpr uint8_t masks54[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words55[]={0x02202025u,0x8E040010u,0x1444001Bu,0x3C040000u,0x8C840000u,0x3C050000u,0x00042080u,0x24A50000u,0x00852021u,0x8C840000u,0x8C840000u,0x28840000u};
inline constexpr uint8_t masks55[]={0,0,0,1,1,1,0,1,0,1,1,0};
inline constexpr uint32_t words56[]={0x8C840000u,0x10000003u,0x00000000u,0x3C040000u,0x8C820000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x3C060000u,0x8CC60000u,0x3405000Au,0x00C5001Bu};
inline constexpr uint8_t masks56[]={0,0,0,1,1,0,0,0,1,1,0,0};
inline constexpr uint32_t words57[]={0xAFB10004u,0x3C110000u,0xAFB00000u,0x00808025u,0x26310000u,0xAFBF0008u,0x0C000000u,0x02202025u,0x8E040010u,0x1444001Bu,0x3C040000u,0x8C840000u};
inline constexpr uint8_t masks57[]={0,1,0,0,1,0,2,0,0,0,1,1};
inline constexpr uint32_t words58[]={0x00073B03u,0x0C000000u,0x00078040u,0x3C110000u,0x8E240000u,0x3C054840u,0x24A58000u,0xAC850000u,0x8E240000u,0x3C054940u,0x24840004u,0xAE240000u};
inline constexpr uint8_t masks58[]={0,2,0,1,1,0,0,0,1,0,0,1};
inline constexpr uint32_t words59[]={0x27BDFE50u,0xAFB00194u,0x00808025u,0xAFB10198u,0x27B10010u,0x86050130u,0xAFB2019Cu,0xAFB301A0u,0xAFBF01A4u,0x0C000000u,0x02202025u,0x8604012Cu};
inline constexpr uint8_t masks59[]={0,0,0,0,0,0,0,0,0,2,0,0};
// Sprite-manager layer loop (0x088E127C); its per-instance draw call at +0x3C.
inline constexpr uint32_t spriteCallWords[]={0x27BDFFF0u,0x000528C0u,0xAFB00004u,0x00858021u,0xE7B40000u,0x46006506u,0x26100008u,0xAFB10008u,0xAFBF000Cu,0x8E040000u,0x02008823u,0x12240008u,0x00000000u,0x8E310004u,0x4600A306u,0x0C000000u,0x02202025u};
inline constexpr uint8_t spriteCallMasks[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0};
// Resolve native code and globals before installing any patches.
inline constexpr uint32_t primitiveWords[]={0x3C0A0000u,0x8C8B0000u,0x00CA3025u,0xAD660000u,0x8C860000u,0x24C60004u,0x11000013u,0xAC860000u,0x00085202u,0x3C0B0000u,0x014B5024u,0x3C0B0000u};
inline constexpr uint8_t primitiveMasks[]={1,1,0,1,1,0,0,1,0,1,0,1};
inline constexpr uint32_t movie0Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x8E041EC0u,0x3C05C900u,0x24A50003u,0xAC850000u,0x8E041EC0u,0x3C0508B5u};
inline constexpr uint8_t movie0Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t movie1Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x8E041EC0u,0x3C052100u,0x24A50001u,0xAC850000u,0x8E041EC0u,0x3C0508B5u};
inline constexpr uint8_t movie1Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t movie2Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x3C0408B5u,0x8C844300u,0x8E051EC0u,0x24841000u,0x00943024u,0x3C07B000u};
inline constexpr uint8_t movie2Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t movie3Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x3C0408B5u,0x8C844300u,0x8E051EC0u,0x24840400u,0x00943024u,0x3C07B000u};
inline constexpr uint8_t movie3Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t movie4Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x3C0408B5u,0x8C844300u,0x8E051EC0u,0x24840800u,0x00943024u,0x3C07B000u};
inline constexpr uint8_t movie4Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t movie5Words[]={0x02403825u,0x02204025u,0x44026000u,0xAFA20000u,0x0C000000u,0xAFA00004u,0x8FA40010u,0x8FA5001Cu,0x24860100u,0x8E041EC0u,0x00C5382Bu,0x14E0FDA0u};
inline constexpr uint8_t movie5Masks[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t padWords[]={0x8C840000u,0x3C050000u,0x00043200u,0x00042180u,0x00C41021u,0x24A40000u,0x03E00008u,0x00441021u};
inline constexpr uint8_t padMasks[]={0,1,0,0,0,1,0,0};
inline constexpr uint32_t hintPartsWords[]={0x0C000000u,0x34060001u,0x3C160000u,0x8ED60000u,0x12C0001Fu,0x00000000u,0x92C60015u,0x24040000u,0x34050000u,0x34070001u,0x8EF70000u,0x50C70001u};
inline constexpr uint8_t hintPartsMasks[]={2,0,1,1,0,0,0,1,0,0,1,0};
inline constexpr uint32_t hintQueueWords[]={0xAFB70000u,0xAFBE0000u,0xAFBF0000u,0x0C000000u,0x24840000u,0x3C040000u,0x3484CCCDu,0x4484A000u,0x3C130000u,0x3C1E0000u,0x0040B825u,0x8E640000u};
inline constexpr uint8_t hintQueueMasks[]={1,1,1,2,1,1,0,0,1,1,0,1};
inline constexpr uint32_t hintMiddleWords[]={0x3C130000u,0x3C1E0000u,0x0040B825u,0x8E640000u,0x12E0010Au,0x27DE0000u,0x8EE50000u,0x10A00107u,0xAFBE0000u,0x92E50022u,0x3406000Cu,0x00052C00u};
inline constexpr uint8_t hintMiddleMasks[]={1,1,0,1,0,1,1,0,1,0,0,0};
inline constexpr std::array<console::AddressRule,72> rules={{
    {0x0884F52Cu,0x0884F51Cu,words0,masks0,12,16,16,0,0,console::AddressKind::Code},
    {0x088559ACu,0x088559ACu,words59,masks59,12,0,0,0,0,console::AddressKind::Code},
    {0x088559F0u,0x088559E0u,words1,masks1,12,44,16,0,0,console::AddressKind::Code},
    {0x08860650u,0x08860640u,movie0Words,movie0Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x08860870u,0x08860860u,movie1Words,movie1Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x08860A14u,0x08860A04u,movie2Words,movie2Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x08860B1Cu,0x08860B0Cu,movie3Words,movie3Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x08860CA8u,0x08860C98u,movie4Words,movie4Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x08860DB0u,0x08860DA0u,movie5Words,movie5Masks,12,0,16,0,0,console::AddressKind::Code},
    {0x088704C8u,0x088704B8u,words2,masks2,12,40,16,0,0,console::AddressKind::Code},
    {0x0888BCACu,0x0888BC9Cu,words3,masks3,12,38,16,0,0,console::AddressKind::Code},
    {0x0888BF9Cu,0x0888BF8Cu,words4,masks4,12,17,16,0,0,console::AddressKind::Code},
    {0x08890E68u,0x08890E50u,words5,masks5,12,46,24,0,0,console::AddressKind::Code},
    {0x08890FE4u,0x08890FD4u,words6,masks6,12,2,16,0,0,console::AddressKind::Code},
    {0x08891160u,0x08891150u,words7,masks7,12,44,16,0,0,console::AddressKind::Code},
    {0x08891414u,0x08891404u,words8,masks8,12,24,16,0,0,console::AddressKind::Code},
    {0x088915D4u,0x088915C4u,words9,masks9,12,42,16,0,0,console::AddressKind::Code},
    {0x088CBFA8u,0x088CBF98u,words10,masks10,12,32,16,0,0,console::AddressKind::Code},
    {0x088CFC1Cu,0x088CFC0Cu,words11,masks11,12,26,16,0,0,console::AddressKind::Code},
    {0x088D8C70u,0x088D8C60u,words12,masks12,12,42,16,0,0,console::AddressKind::Code},
    {0x088D90E0u,0x088D90D0u,words13,masks13,12,4,16,0,0,console::AddressKind::Code},
    {0x088E12B8u,0x088E127Cu,spriteCallWords,spriteCallMasks,17,20,0x3C,0,0,console::AddressKind::Code},
    {0x089100C0u,0x089100C0u,primitiveWords,primitiveMasks,12,0,0,0,0,console::AddressKind::Code},
    {0x0896E6F4u,0x0896E6E4u,words14,masks14,12,2,16,0,0,console::AddressKind::Code},
    {0x0896EA44u,0x0896EA34u,words15,masks15,12,34,16,0,0,console::AddressKind::Code},
    {0x08988CA4u,0x08988C94u,words16,masks16,12,21,16,0,0,console::AddressKind::Code},
    {0x0898D388u,0x0898D378u,words18,masks18,12,42,16,0,0,console::AddressKind::Code},
    {0x08992490u,0x08992480u,words19,masks19,12,8,16,0,0,console::AddressKind::Code},
    {0x089924B0u,0x089924A0u,words20,masks20,12,8,16,0,0,console::AddressKind::Code},
    {0x089C052Cu,0x089C051Cu,words21,masks21,12,12,16,0,0,console::AddressKind::Code},
    {0x089C0E18u,0x089C0E08u,words22,masks22,12,17,16,0,0,console::AddressKind::Code},
    {0x089C650Cu,0x089C64FCu,words23,masks23,12,36,16,0,0,console::AddressKind::Code},
    {0x089C66B0u,0x089C66A0u,words24,masks24,12,42,16,0,0,console::AddressKind::Code},
    {0x089C8200u,0x089C81F0u,words25,masks25,12,4,16,0,0,console::AddressKind::Code},
    {0x089C8274u,0x089C8264u,words26,masks26,12,16,16,0,0,console::AddressKind::Code},
    {0x089CAE2Cu,0x089CAE1Cu,words27,masks27,12,2,16,0,0,console::AddressKind::Code},
    {0x089CB77Cu,0x089CB76Cu,words28,masks28,12,17,16,0,0,console::AddressKind::Code},
    {0x089CC124u,0x089CC114u,words29,masks29,12,17,16,0,0,console::AddressKind::Code},
    {0x089CCA70u,0x089CCA60u,words30,masks30,12,17,16,0,0,console::AddressKind::Code},
    {0x089CD7ECu,0x089CD7DCu,words31,masks31,12,38,16,0,0,console::AddressKind::Code},
    {0x089CDB38u,0x089CDB28u,words32,masks32,12,2,16,0,0,console::AddressKind::Code},
    {0x089CDDF8u,0x089CDDE8u,words33,masks33,12,0,16,0,0,console::AddressKind::Code},
    {0x089CE2F0u,0x089CE2E0u,words34,masks34,12,17,16,0,0,console::AddressKind::Code},
    {0x089CE8B8u,0x089CE8A8u,words35,masks35,12,0,16,0,0,console::AddressKind::Code},
    {0x089CEC10u,0x089CEC00u,words36,masks36,12,2,16,0,0,console::AddressKind::Code},
    {0x089CF94Cu,0x089CF93Cu,words37,masks37,12,2,16,0,0,console::AddressKind::Code},
    {0x089CFA4Cu,0x089CFA3Cu,words38,masks38,12,16,16,0,0,console::AddressKind::Code},
    {0x089CFCE8u,0x089CFCD8u,words39,masks39,12,44,16,0,0,console::AddressKind::Code},
    {0x089CFE54u,0x089CFE44u,words40,masks40,12,42,16,0,0,console::AddressKind::Code},
    {0x089D0340u,0x089D0330u,words41,masks41,12,26,16,0,0,console::AddressKind::Code},
    {0x089D0D28u,0x089D0D18u,words42,masks42,12,42,16,0,0,console::AddressKind::Code},
    {0x089D10E4u,0x089D10D4u,words43,masks43,12,17,16,0,0,console::AddressKind::Code},
    {0x089E7270u,0x089E7270u,radarWords,radarMasks,10,0,0,0,0,console::AddressKind::Code},
    {0x089FC710u,0x089FC700u,words44,masks44,12,34,16,0,0,console::AddressKind::Code},
    {0x089FC98Cu,0x089FC97Cu,words45,masks45,12,42,16,0,0,console::AddressKind::Code},
    {0x08A05F28u,0x08A05F18u,words46,masks46,12,14,16,0,0,console::AddressKind::Code},
    {0x08A2ACACu,0x08A2AC9Cu,words47,masks47,12,16,16,0,0,console::AddressKind::Code},
    {0x08A2ACE8u,0x08A2ACD8u,words48,masks48,12,16,16,0,0,console::AddressKind::Code},
    {0x08A2AD24u,0x08A2AD14u,words49,masks49,12,0,16,0,0,console::AddressKind::Code},
    {0x08A36E2Cu,0x08A36E1Cu,words50,masks50,12,2,16,0,0,console::AddressKind::Code},
    {0x08A6DD1Cu,0x08A6DD0Cu,words51,masks51,12,30,16,0,0,console::AddressKind::Code},
    {0x08A6DE68u,0x08A6DE58u,words52,masks52,12,44,16,0,0,console::AddressKind::Code},
    {0x08AC79A4u,0x08AC7994u,words53,masks53,12,38,16,0,0,console::AddressKind::Code},
    {0x08AC7C50u,0x08AC7C40u,words54,masks54,12,36,16,0,0,console::AddressKind::Code},
    {0x08B5B238u,0x0883DEA8u,words55,masks55,12,8,16,12,0,console::AddressKind::Absolute},
    {0x08B5B9FCu,0x08A1884Cu,hintPartsWords,hintPartsMasks,12,7,12,8,0,console::AddressKind::Absolute},
    {0x08C0D160u,0x08817150u,words56,masks56,12,44,16,12,0,console::AddressKind::Absolute},
    {0x08C11258u,0x0883DE8Cu,words57,masks57,12,36,16,4,0,console::AddressKind::Absolute},
    {0x08C11EC0u,0x0883E518u,words58,masks58,12,1,16,12,0,console::AddressKind::Absolute},
    {0x08D57280u,0x08973B78u,padWords,padMasks,8,10,20,4,0,console::AddressKind::Absolute},
    {0x08D58F88u,0x08A18484u,hintQueueWords,hintQueueMasks,12,27,16,-52,0,console::AddressKind::Absolute},
    {0x08D59010u,0x08A184A4u,hintMiddleWords,hintMiddleMasks,12,11,20,4,0,console::AddressKind::Absolute},
}};
}
inline console::NativeProfile<profile_data::rules> nativeProfile;
template<uintptr_t Reference> uintptr_t Address() { return nativeProfile.template get<Reference>(); }
inline bool InitializeAddresses() {
    sceKernelIcacheInvalidateRange(reinterpret_cast<void*>(pattern.text_addr),pattern.text_size);
    return nativeProfile.initialize({pattern.text_addr,reinterpret_cast<const uint8_t*>(pattern.text_addr),pattern.text_size,console::portable::gameGP});
}
}
