/**************************************
File name     : TAGATTR.H
Function      : Implement EMV2004 standard Tag Attributes obtain
Author        : Yu Jun
First edition : Mar 4th, 2003
Note          : Refer EMV2004 Specification Book3 Part IV, Annex B
                                                Part IV, Annex A
**************************************/
#ifndef _TAGATTR_H
#define _TAGATTR_H

#define TAG_ATTR_N          0    // compacted, right aligned
#define TAG_ATTR_CN         1    // compacted number, left aligned
#define TAG_ATTR_B          2    // binary, var 类型也归到此类
#define TAG_ATTR_A          3    // Alpha
#define TAG_ATTR_AN         4    // Alpha Numeric
#define TAG_ATTR_ANS        5    // Alpha Numeric Special
#define TAG_ATTR_UNKNOWN    0xff // unknown tag

#define TAG_FROM_CARD       0    // 来自卡片
#define TAG_FROM_TERM       1    // 来自终端
#define TAG_FROM_ISSUER     2    // 来自发卡行
#define TAG_FROM_UNKNOWN    0xff // unknown tag

// function : get attribute of the Tag
// In  : psTag : tag
// Ret : TAG_ATTR_N or TAG_ATTR_CN or TAG_ATTR_B or TAG_ATTR_A or TAG_ATTR_AN or TAG_ATTR_ANS
//       or TAG_ATTR_UNKNOWN
unsigned int uiTagAttrGetType(unsigned char *psTag);

// function : get origin of the Tag
// In  : psTag : tag
// Ret : TAG_FROM_CARD or TAG_FROM_TERM or TAG_FROM_ISSUER
//       or TAG_FROM_UNKNOWN
unsigned int uiTagAttrGetFrom(unsigned char *psTag);

// function : get attribute of the Tag
// In  : psTag   : tag
// Out : piLeast : least length available, 以实际保存长度为单位
//       piMost  : most length available, 以实际保存长度为单位
// Ret : TAG_ATTR_N or TAG_ATTR_CN or TAG_ATTR_B or TAG_ATTR_A or TAG_ATTR_AN or TAG_ATTR_ANS
//       or TAG_ATTR_UNKNOWN
unsigned int uiTagAttrGetRange(unsigned char *psTag, int *piLeast, int *piMost);

// function : get description of the Tag
// In  : psTag : tag
// Ret : description or ""
unsigned char *psTagAttrGetDesc(unsigned char *psTag);

# endif