#pragma once
#include "../Shared/Console/NativeProfile.hpp"
#include "../Shared/Console/PSP.hpp"

namespace lcsfx {
namespace profile_data {
inline constexpr uint32_t words0[]={0x3C0442C8u,0x4480A000u,0x4484B000u,0x3C044120u,0x0C000000u,0x4484F000u,0x8EC5BF04u,0x34040001u,0x0C000000u,0x8CA50000u,0xE7B60080u,0xE7B40084u};
inline constexpr uint8_t masks0[]={0,0,0,0,2,0,0,0,2,0,0,0};
inline constexpr uint32_t words1[]={0x00001025u,0x34020002u,0x03E00008u,0x00000000u,0x27BDFFF0u,0x3C040000u,0xC48C0000u,0x3C043F8Cu,0x3484CCCDu,0x44846800u,0xAFBF0000u,0x460D603Eu};
inline constexpr uint8_t masks1[]={0,0,0,0,0,1,1,0,0,0,0,0};
inline constexpr uint32_t words2[]={0x00000000u,0x8FBF0000u,0x03E00008u,0x27BD0010u,0x27BDFFF0u,0x3C040000u,0xC48C0000u,0x3C043F8Cu,0x3484CCCDu,0x44846800u,0xAFBF0000u,0x460D603Eu};
inline constexpr uint8_t masks2[]={0,0,0,0,0,1,1,0,0,0,0,0};
inline constexpr uint32_t words3[]={0x34020001u,0x00001025u,0x03E00008u,0x00000000u,0x27BDFFE0u,0x24840000u,0xC48D0000u,0xAFBF0010u,0x0C000000u,0xC48C0000u,0x46000306u,0x44806800u};
inline constexpr uint8_t masks3[]={0,0,0,0,0,1,1,0,2,1,0,0};
inline constexpr uint32_t words4[]={0x8FB20008u,0x8FBF000Cu,0x03E00008u,0x27BD0010u,0x27BDFA10u,0x3C053F75u,0xC48C0028u,0x34A5C28Fu,0x44856800u,0xE7B405B8u,0x460D603Cu,0xE7B605BCu};
inline constexpr uint8_t masks4[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words5[]={0x44076800u,0xAFA0000Cu,0x44886800u,0x30E700FFu,0x0C000000u,0x340800FFu,0x1000002Cu,0x00000000u,0x44946000u,0x8C840000u,0x44937000u,0x3C053F33u};
inline constexpr uint8_t masks5[]={0,0,0,0,2,0,0,0,0,1,0,0};
inline constexpr uint32_t words6[]={0x00005825u,0x44886800u,0x30E700FFu,0x340800FFu,0x0C000000u,0xAFA0000Cu,0x27A40050u,0xD8800000u,0x27B20090u,0xFA400000u,0x26130020u,0xDA600000u};
inline constexpr uint8_t masks6[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t words7[]={0x44076800u,0xAFA0000Cu,0x44886800u,0x30E700FFu,0x0C000000u,0x340800FFu,0x100002BEu,0x00000000u,0x86050058u,0x3C070000u,0x8CE70000u,0x94E70000u};
inline constexpr uint8_t masks7[]={0,0,0,0,2,0,0,0,0,1,1,1};
inline constexpr uint32_t words8[]={0x44076800u,0xAFA0000Cu,0x44886800u,0x30E700FFu,0x0C000000u,0x340800FFu,0x1000002Bu,0x00000000u,0x44916000u,0x8C840000u,0x44927000u,0x3C053F33u};
inline constexpr uint8_t masks8[]={0,0,0,0,2,0,0,0,0,1,0,0};
inline constexpr uint32_t words9[]={0x44076800u,0xAFA0000Cu,0x44886800u,0x30E700FFu,0x0C000000u,0x340800FFu,0xC7B405B8u,0xC7B605BCu,0xC7B805C0u,0x8FB005C4u,0x8FB105C8u,0x8FB205CCu};
inline constexpr uint8_t masks9[]={0,0,0,0,2,0,0,0,0,0,0,0};
inline constexpr uint32_t words10[]={0xACA40000u,0x8FBF0020u,0x03E00008u,0x27BD0030u,0x27BDFFE0u,0x00803025u,0x00A02025u,0x24C5FFFFu,0x2CA6000Fu,0xAFBF0014u,0x10C00062u,0x00000000u};
inline constexpr uint8_t masks10[]={1,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words11[]={0x00C53021u,0x00052880u,0xAFB20028u,0x00C52821u,0x3C120000u,0xAFB10024u,0xAFB3002Cu,0x460D603Cu,0x00A42821u,0x26520000u,0x3C110000u,0x3C130000u};
inline constexpr uint8_t masks11[]={0,0,0,0,1,0,0,0,0,1,1,1};
inline constexpr uint32_t words12[]={0xAFB10024u,0xAFB3002Cu,0x460D603Cu,0x00A42821u,0x26520000u,0x3C110000u,0x3C130000u,0xAFBF0030u,0x45030001u,0x46006346u,0xE60D0000u,0x90A601A8u};
inline constexpr uint8_t masks12[]={0,0,0,0,1,1,1,0,0,0,1,0};
inline constexpr uint32_t words13[]={0x00000000u,0x0C000000u,0x02402025u,0x26100000u,0x2A040038u,0x1480FFF8u,0x26520000u,0x8FB00020u,0x8FB10024u,0x8FB20028u,0x8FB3002Cu,0x8FBF0030u};
inline constexpr uint8_t masks13[]={0,2,0,1,0,0,1,0,0,0,0,0};
inline constexpr uint32_t words14[]={0x1480FFF2u,0x00000000u,0x0C000000u,0x00000000u,0x3C040000u,0x34050000u,0x24840000u,0x000531C0u,0x00053900u,0x00C73023u,0x00C43021u,0x24A50001u};
inline constexpr uint8_t masks14[]={0,0,2,0,1,0,1,0,0,0,0,0};
inline constexpr uint32_t words15[]={0x0C000000u,0x00000000u,0x3C040000u,0x34050000u,0x24840000u,0x000531C0u,0x00053900u,0x00C73023u,0x00C43021u,0x24A50001u,0xACC00010u,0x30A5FFFFu};
inline constexpr uint8_t masks15[]={2,0,1,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words16[]={0x00C43021u,0x24A50001u,0xACC00010u,0x30A5FFFFu,0x28A60038u,0x14C0FFF7u,0x00000000u,0x8FB00010u,0x8FB10014u,0x8FB20018u,0x8FB3001Cu,0x8FBF0020u};
inline constexpr uint8_t masks16[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words17[]={0x8FB3001Cu,0x8FBF0020u,0x03E00008u,0x27BD0030u,0x27BDFFB0u,0x8FA20050u,0x314A00FFu,0x8FA30054u,0x3C0E0000u,0x8FAC0058u,0x000A5080u,0x25CE0000u};
inline constexpr uint8_t masks17[]={0,0,0,0,0,0,0,0,1,0,0,1};
inline constexpr uint32_t words18[]={0x460C703Eu,0x00000000u,0x45000006u,0x00000000u,0x3C060000u,0x34070000u,0x34080001u,0x10000003u,0x24C60000u,0x1000001Du,0x00000000u,0x1100000Bu};
inline constexpr uint8_t masks18[]={0,0,0,0,1,0,0,0,1,0,0,0};
inline constexpr uint32_t words19[]={0x3C060000u,0x34070000u,0x34080001u,0x10000003u,0x24C60000u,0x1000001Du,0x00000000u,0x1100000Bu,0x000741C0u,0x00074900u,0x01094023u,0x01064021u};
inline constexpr uint8_t masks19[]={1,0,0,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words20[]={0x00000000u,0x24E70001u,0x30E7FFFFu,0x1000FFF5u,0x28E80038u,0x34040038u,0x10E4000Bu,0x00000000u,0x000721C0u,0x00073900u,0x00873823u,0x00E63821u};
inline constexpr uint8_t masks20[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words21[]={0x24E70001u,0x30E7FFFFu,0x1000FFF5u,0x28E80038u,0x34040038u,0x10E4000Bu,0x00000000u,0x000721C0u,0x00073900u,0x00873823u,0x00E63821u,0x90E40034u};
inline constexpr uint8_t masks21[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words22[]={0x000527C2u,0x02442021u,0x0004A043u,0x34040008u,0x0C000000u,0x00002825u,0x3404000Cu,0x0C000000u,0x34050001u,0x3404000Au,0x0C000000u,0x34050002u};
inline constexpr uint8_t masks22[]={0,0,0,0,2,0,0,2,0,0,2,0};
inline constexpr uint32_t words23[]={0xAFA4006Cu,0x4493D000u,0x4494C000u,0x3C160000u,0x3C120000u,0x24A50000u,0xAFB00064u,0x3C0440C0u,0x4680D6A0u,0x4680C620u,0x341E0000u,0x4484E000u};
inline constexpr uint8_t masks23[]={0,0,0,1,1,1,0,0,0,0,0,0};
inline constexpr uint32_t words24[]={0x341E0000u,0x4484E000u,0x26D60000u,0x341400FFu,0x26520000u,0x34170006u,0xAFA50068u,0x001E69C0u,0x001E2100u,0x01A46823u,0x01B28821u,0x340C0005u};
inline constexpr uint8_t masks24[]={0,0,1,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words25[]={0xA2240038u,0x27C40001u,0x0004F400u,0x001EF403u,0x2BC40038u,0x1480FE4Bu,0x00000000u,0x34040008u,0x0C000000u,0x34050001u,0x34040006u,0x0C000000u};
inline constexpr uint8_t masks25[]={0,0,0,0,0,0,0,0,2,0,0,2};
inline constexpr uint32_t words26[]={0x00000000u,0x26040001u,0x00048400u,0x00108403u,0x2A040038u,0x1480FF9Au,0x00000000u,0x3404000Cu,0x0C000000u,0x00002825u,0x34040006u,0x0C000000u};
inline constexpr uint8_t masks26[]={0,0,0,0,0,0,0,0,2,0,0,2};
inline constexpr uint32_t words27[]={0xC6CC0000u,0xE7BA00C0u,0x4480D000u,0xAFB500E0u,0x3C150000u,0x461A603Eu,0xE7B400B4u,0xE7B600B8u,0xE7B800BCu,0xE7BC00C4u,0xE7BE00C8u,0xAFB000CCu};
inline constexpr uint8_t masks27[]={1,0,0,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words28[]={0xAFB700E8u,0xAFBE00ECu,0xAFBF00F0u,0x45000012u,0x26B50000u,0x34050000u,0x2404FFBFu,0x000531C0u,0x00053900u,0x00C73023u,0x00D53021u,0x80C70038u};
inline constexpr uint8_t masks28[]={0,0,0,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words29[]={0x00E43824u,0x00052C00u,0xA0C70038u,0x00052C03u,0x28A60038u,0x14C0FFF5u,0x000531C0u,0x10000114u,0x00000000u,0x3404000Eu,0x0C000000u,0x00002825u};
inline constexpr uint8_t masks29[]={0,0,0,0,0,0,0,0,0,0,2,0};
inline constexpr uint32_t words30[]={0x26240001u,0x00048C00u,0x00118C03u,0x3C05C47Au,0x2A240038u,0x1480FF2Cu,0x44856000u,0x0C000000u,0x00000000u,0x3404000Au,0x0C000000u,0x34050005u};
inline constexpr uint8_t masks30[]={0,0,0,0,0,0,0,2,0,0,2,0};
inline constexpr uint32_t words31[]={0x46109C02u,0x4600840Du,0x44028000u,0x304200FFu,0x3C0E0000u,0x340F0000u,0x34190001u,0x34180038u,0x25CE0000u,0x1320000Bu,0x000FC9C0u,0x000F8100u};
inline constexpr uint8_t masks31[]={0,0,0,0,1,0,0,0,1,0,0,0};
inline constexpr uint32_t words32[]={0x304200FFu,0x3C0E0000u,0x340F0000u,0x34190001u,0x34180038u,0x25CE0000u,0x1320000Bu,0x000FC9C0u,0x000F8100u,0x0330C823u,0x032EC821u,0x8F390010u};
inline constexpr uint8_t masks32[]={0,1,0,0,0,1,0,0,0,0,0,0};
inline constexpr uint32_t words33[]={0x3C0E0000u,0x340F0000u,0x34190001u,0x34180038u,0x25CE0000u,0x1320000Bu,0x000FC9C0u,0x000F8100u,0x0330C823u,0x032EC821u,0x8F390010u,0x13240005u};
inline constexpr uint8_t masks33[]={1,0,0,0,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words34[]={0x00000000u,0x25EF0001u,0x31EFFFFFu,0x1000FFF5u,0x29F90038u,0x15F80037u,0x00000000u,0x10400004u,0x00000000u,0x340F0000u,0x10000003u,0x34190001u};
inline constexpr uint8_t masks34[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words35[]={0x00000000u,0x25EF0001u,0x31EFFFFFu,0x1000FFF5u,0x29F90038u,0x15F80003u,0x000FC9C0u,0x10000059u,0x00000000u,0x000F7900u,0x032FC823u,0x032EC021u};
inline constexpr uint8_t masks35[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words36[]={0x460C7303u,0x46006000u,0x03E00008u,0x00000000u,0x27BDFFC0u,0xE7B40020u,0xE7B60024u,0xAFB00028u,0xAFB1002Cu,0xAFB20030u,0xAFB30034u,0xAFB40038u};
inline constexpr uint8_t masks36[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words37[]={0x44846000u,0x3C040000u,0x03E00008u,0xE48C0000u,0x27BDFFD0u,0xAFB20028u,0x3C120000u,0x8E440000u,0xAFB00020u,0xAFB10024u,0xAFBF002Cu,0x18800022u};
inline constexpr uint8_t masks37[]={0,1,0,1,0,0,1,1,0,0,0,0};
inline constexpr uint32_t words38[]={0xA0860006u,0x90A50003u,0x03E00008u,0xA0850007u,0x27BDFF80u,0xAFB00058u,0x309000FFu,0xAFB30064u,0x00079C00u,0x3C043F00u,0xE7B40054u,0xAFB1005Cu};
inline constexpr uint8_t masks38[]={0,0,0,0,0,0,0,0,0,0,0,0};
inline constexpr uint32_t words39[]={0x10400033u,0x00000000u,0x3C040000u,0xC7B40058u,0x24840000u,0x4614D503u,0x8C850000u,0x34040001u,0x0C000000u,0x8CA50000u,0x3C040000u,0x8C850000u};
inline constexpr uint8_t masks39[]={0,0,1,0,1,0,1,0,2,0,1,1};
inline constexpr uint32_t words40[]={0x46007346u,0x0C000000u,0x00000000u,0x3C040000u,0x8C840000u,0x44846000u,0x0C000000u,0x46806320u,0x0C000000u,0x00000000u,0x3C04437Fu,0x44846000u};
inline constexpr uint8_t masks40[]={0,2,0,1,1,0,2,0,2,0,0,0};
inline constexpr uint32_t words41[]={0x3C150000u,0x38840001u,0x0004202Bu,0xA2040000u,0x8EA40000u,0x34110000u,0x3C054220u,0x0224302Au,0x4485A000u,0x10C0004Au,0x3C140000u,0x27B20034u};
inline constexpr uint8_t masks41[]={1,0,0,1,1,0,0,0,0,0,1,0};
inline constexpr uint32_t words42[]={0x27BD0030u,0x27BDFF00u,0x8C850000u,0x3C060000u,0x8CC60000u,0x3C073EB3u,0x00C5282Bu,0x34E73333u,0x3C063F80u,0xE7B400C4u,0x44866000u,0x4487A000u};
inline constexpr uint8_t masks42[]={0,0,1,1,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words43[]={0x46806320u,0x44846800u,0x460D6303u,0x3C040000u,0xC48E0000u,0x460C703Eu,0x00000000u,0x4502000Fu,0x34100001u,0x96240056u,0x3C054743u,0x44846000u};
inline constexpr uint8_t masks43[]={0,0,0,1,1,0,0,0,0,0,0,0};
inline constexpr uint32_t words44[]={0x00000000u,0x3C100000u,0x92040000u,0x3C110000u,0x92250000u,0x00053180u,0x00052880u,0x00C52823u,0x00852021u,0x44846000u,0x46806320u,0x3C040000u};
inline constexpr uint8_t masks44[]={0,1,1,1,1,0,0,0,0,0,0,1};
inline constexpr uint32_t words45[]={0x02002025u,0x0C000000u,0x00000000u,0x3C100000u,0x92040000u,0x3C110000u,0x92250000u,0x00053180u,0x00052880u,0x00C52823u,0x00852021u,0x44846000u};
inline constexpr uint8_t masks45[]={0,2,0,1,1,1,1,0,0,0,0,0};
inline constexpr uint32_t words46[]={0x00000000u,0x340500B4u,0xC6AE0000u,0x00A42023u,0xC6EF0000u,0x44846000u,0x460F703Cu,0x00000000u,0x45000007u,0x46806320u,0xC6EE0000u,0x460ED381u};
inline constexpr uint8_t masks46[]={0,0,1,0,1,0,0,0,0,0,1,0};
inline constexpr uint32_t words47[]={0x03E00008u,0x00000000u,0x27BDFF90u,0x3C050000u,0x24A50000u,0x24A50000u,0xD8A00000u,0x27A50030u,0xF8A00000u,0xD8800000u,0xD8A10000u,0x6481801Cu};
inline constexpr uint8_t masks47[]={0,0,0,1,1,1,1,0,1,0,1,0};
// US reference sites; instruction operands resolve the matching native
// addresses in other executable revisions before any patches are installed.
inline constexpr uint32_t arenaWords[]={0xAFBF0034u,0x0C000000u,0x02002825u,0x3C140000u,0x26950000u,0x8EA4000Cu,0x00103100u,0x2416FFF0u,0x00869823u,0x02769824u,0xAEB3000Cu,0x02602025u};
inline constexpr uint8_t arenaMasks[]={0,2,0,1,1,0,0,0,0,0,0,0};
inline constexpr uint32_t pathsGlobalWords[]={0x03E00008u,0x00000000u,0x8C860000u,0x3C050000u,0x8CA50000u,0x00C63821u,0x8CA50000u,0x00C73021u,0x00063080u,0x00A63021u,0x80C60009u,0x30C60004u};
inline constexpr uint8_t pathsGlobalMasks[]={0,0,1,1,1,0,1,0,0,0,0,0};
inline constexpr uint32_t im3dTransformWords[]={0x27BDFFD0u,0xAFB10014u,0x00A08825u,0xAFB00010u,0xAFB20018u,0xAFB3001Cu,0xAFBF0020u,0x10A00014u,0x00C08025u,0x3C050000u,0x24A70000u,0x8CE5000Cu};
inline constexpr uint8_t im3dTransformMasks[]={0,0,0,0,0,0,0,0,0,1,1,0};
inline constexpr uint32_t im3dIndexedWords[]={0x27BDFFD0u,0xAFB3001Cu,0x3C130000u,0x8E670000u,0xAFB00010u,0xAFB10014u,0x00808025u,0x00A08825u,0xAFB20018u,0xAFB40020u,0xAFB50024u,0xAFB60028u};
inline constexpr uint8_t im3dIndexedMasks[]={0,0,1,1,0,0,0,0,0,0,0,0};
inline constexpr uint32_t im3dEndWords[]={0x3C040000u,0xAC800000u,0x3C040000u,0xAC800000u,0x03E00008u,0x34020001u,0x27BDFFD0u,0xAFB3001Cu,0x3C130000u,0x8E670000u,0xAFB00010u,0xAFB10014u};
inline constexpr uint8_t im3dEndMasks[]={1,1,1,1,0,0,0,0,1,1,0,0};
inline constexpr uint32_t dummyWords[]={0x00039D01u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000037u,0x002D107Cu,0x08000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
inline constexpr uint8_t dummyMasks[]={0,0,0,0,0,0,0,2,0,0,0,0};
inline constexpr uint32_t farClipWords[]={0x311300FFu,0x3C040000u,0xC4960000u,0x3C040000u,0xC4940000u,0x27B40010u,0x3C040000u,0x24850000u,0x24A51AD0u,0x0C000000u,0x02802025u,0xDA800000u};
inline constexpr uint8_t farClipMasks[]={0,1,1,1,1,0,1,1,0,2,0,0};
inline constexpr std::array<console::AddressRule,54> rules={{
    {0x08836EC8u,0x08836EB8u,words0,masks0,12,0,16,0,0,console::AddressKind::Code},
    {0x08865A58u,0x08865A48u,words1,masks1,12,32,16,0,0,console::AddressKind::Code},
    {0x08865ABCu,0x08865AACu,words2,masks2,12,32,16,0,0,console::AddressKind::Code},
    {0x08865BCCu,0x08865BBCu,words3,masks3,12,26,16,0,0,console::AddressKind::Code},
    {0x0886621Cu,0x0886620Cu,words4,masks4,12,29,16,0,0,console::AddressKind::Code},
    {0x08866928u,0x08866918u,words5,masks5,12,42,16,0,0,console::AddressKind::Code},
    {0x088669DCu,0x088669CCu,words6,masks6,12,39,16,0,0,console::AddressKind::Code},
    {0x0886722Cu,0x0886721Cu,words7,masks7,12,24,16,0,0,console::AddressKind::Code},
    {0x08867C78u,0x08867C68u,words8,masks8,12,34,16,0,0,console::AddressKind::Code},
    {0x08867D28u,0x08867D18u,words9,masks9,12,28,16,0,0,console::AddressKind::Code},
    {0x08868560u,0x08868560u,im3dTransformWords,im3dTransformMasks,12,42,0,0,0,console::AddressKind::Code},
    {0x08868844u,0x08868844u,im3dEndWords,im3dEndMasks,12,38,0,0,0,console::AddressKind::Code},
    {0x0886885Cu,0x0886885Cu,im3dIndexedWords,im3dIndexedMasks,12,14,0,0,0,console::AddressKind::Code},
    {0x088B7A70u,0x088B7A60u,words10,masks10,12,40,16,0,0,console::AddressKind::Code},
    {0x089F9ED0u,0x089F9EC0u,words11,masks11,12,38,16,0,0,console::AddressKind::Code},
    {0x089F9EE4u,0x089F9ED4u,words12,masks12,12,18,16,0,0,console::AddressKind::Code},
    {0x089F9F94u,0x089F9F84u,words13,masks13,12,26,16,0,0,console::AddressKind::Code},
    {0x089FA048u,0x089FA038u,words14,masks14,12,0,16,0,0,console::AddressKind::Code},
    {0x089FA050u,0x089FA040u,words15,masks15,12,25,16,0,0,console::AddressKind::Code},
    {0x089FA070u,0x089FA060u,words16,masks16,12,20,16,0,0,console::AddressKind::Code},
    {0x089FA098u,0x089FA088u,words17,masks17,12,46,16,0,0,console::AddressKind::Code},
    {0x089FA150u,0x089FA140u,words18,masks18,12,36,16,0,0,console::AddressKind::Code},
    {0x089FA160u,0x089FA150u,words19,masks19,12,20,16,0,0,console::AddressKind::Code},
    {0x089FA198u,0x089FA188u,words20,masks20,12,37,16,0,0,console::AddressKind::Code},
    {0x089FA19Cu,0x089FA18Cu,words21,masks21,12,33,16,0,0,console::AddressKind::Code},
    {0x089FB164u,0x089FB154u,words22,masks22,12,0,16,0,0,console::AddressKind::Code},
    {0x089FB1E0u,0x089FB1D0u,words23,masks23,12,33,16,0,0,console::AddressKind::Code},
    {0x089FB208u,0x089FB1F8u,words24,masks24,12,10,16,0,0,console::AddressKind::Code},
    {0x089FB8E0u,0x089FB8D0u,words25,masks25,12,20,16,0,0,console::AddressKind::Code},
    {0x089FBADCu,0x089FBACCu,words26,masks26,12,20,16,0,0,console::AddressKind::Code},
    {0x089FBB70u,0x089FBB60u,words27,masks27,12,6,16,0,0,console::AddressKind::Code},
    {0x089FBBB0u,0x089FBBA0u,words28,masks28,12,42,16,0,0,console::AddressKind::Code},
    {0x089FBBE4u,0x089FBBD4u,words29,masks29,12,20,16,0,0,console::AddressKind::Code},
    {0x089FBFF4u,0x089FBFE4u,words30,masks30,12,12,16,0,0,console::AddressKind::Code},
    {0x089FCA3Cu,0x089FCA2Cu,words31,masks31,12,34,16,0,0,console::AddressKind::Code},
    {0x089FCA48u,0x089FCA38u,words32,masks32,12,22,16,0,0,console::AddressKind::Code},
    {0x089FCA4Cu,0x089FCA3Cu,words33,masks33,12,18,16,0,0,console::AddressKind::Code},
    {0x089FCA7Cu,0x089FCA6Cu,words34,masks34,12,6,16,0,0,console::AddressKind::Code},
    {0x089FCAD0u,0x089FCAC0u,words35,masks35,12,6,16,0,0,console::AddressKind::Code},
    {0x08A25970u,0x08A25960u,words36,masks36,12,1,16,0,0,console::AddressKind::Code},
    {0x08A25F64u,0x08A25F54u,words37,masks37,12,44,16,0,0,console::AddressKind::Code},
    {0x08A27098u,0x08A27088u,words38,masks38,12,33,16,0,0,console::AddressKind::Code},
    {0x08B3980Cu,0x08865B5Cu,pathsGlobalWords,pathsGlobalMasks,12,40,16,12,0,console::AddressKind::Absolute},
    {0x08B4BF04u,0x08836B4Cu,words39,masks39,12,21,16,8,0,console::AddressKind::Absolute},
    {0x08B56ACCu,0x088266ACu,words40,masks40,12,40,16,12,0,console::AddressKind::Absolute},
    {0x08B56AD0u,0x0882A934u,words41,masks41,12,36,16,0,0,console::AddressKind::Absolute},
    {0x08B5E144u,0x08807828u,words42,masks42,12,28,16,12,0,console::AddressKind::Absolute},
    {0x08B5E184u,0x08819314u,words43,masks43,12,41,16,12,0,console::AddressKind::Absolute},
    {0x08B5E1A8u,0x088369DCu,words44,masks44,12,21,16,12,0,console::AddressKind::Absolute},
    {0x08B5E1A9u,0x088369D4u,words45,masks45,12,29,16,12,0,console::AddressKind::Absolute},
    {0x08B5E208u,0x08836AB4u,words46,masks46,12,18,16,-120,0,console::AddressKind::Absolute},
    {0x08B5E244u,0x08A259A4u,farClipWords,farClipMasks,12,10,16,12,0,console::AddressKind::Absolute},
    {0x08B833A0u,0x08806EC8u,words47,masks47,12,42,16,12,0,console::AddressKind::Absolute},
    {0x08B8ECD0u,0x0886827Cu,arenaWords,arenaMasks,12,38,16,12,0,console::AddressKind::Absolute},
}};
}
inline console::NativeProfile<profile_data::rules> nativeProfile;
template<uintptr_t Reference> uintptr_t Address() { return nativeProfile.template get<Reference>(); }
inline uintptr_t Address(uintptr_t reference) { return nativeProfile.get(reference); }
inline bool InitializeAddresses() {
    sceKernelIcacheInvalidateRange(reinterpret_cast<void*>(pattern.text_addr),pattern.text_size);
    return nativeProfile.initialize({pattern.text_addr,reinterpret_cast<const uint8_t*>(pattern.text_addr),pattern.text_size,console::portable::gameGP});
}
}
