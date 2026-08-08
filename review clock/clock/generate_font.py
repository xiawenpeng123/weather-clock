#!/usr/bin/env python3
"""
中文字体字模生成器 - 16x16 点阵
适用于 SSD1306 OLED (Adafruit_GFX 格式)
用法: py -3 generate_font.py
输出: 可直接复制到 Arduino .ino 文件的 PROGMEM 数组
"""

from PIL import Image, ImageFont, ImageDraw
import os

# 需要生成字模的所有汉字 (全中国二字地名所含字符, 共584字)
CHARS = (  # 1225 chars
    "丁七万丈三上下且丘业丛东两个中丰临丹为主丽乃久义乌乐九习乡乳乾二于云互五井亚交亨"
    "京亭亳什仁仆介从仑仓仔仙代令仪们仲任伊伍休会伟伦伯伽余佛作佳依侯保信修偃偏儋儿元"
    "充光克兖党全八公六兰共关兴兵冀内冈册冕军农冠冲冶冷准凉凌凤凭凯凰刀分则刚利前剑力"
    "劝功加务助勃勉勐勒勤匀包化北区十千华卓单南博卡卢卧卫印即厂历原厢厦县友双叙叠口古"
    "句召台右叶各合吉同名后吐向吕君含启吴吾呈周呼和咸哈响唐商喀善喇喜嘉嘎嘴噶囊四回团"
    "园围固国图圈土圣圩地圳场坂坊坎坛坝坡坤坪坻垒垣垦垫埇城埔埗埠基堂堆堡堰塔塘塞填墅"
    "增墨壁壤壶复夏外多大天太央头夷夹奇奈奉奎好如妃始姑姚姜威娄婺嫩子孙孚孜孝孟宁宇安"
    "宏宕宗官定宛宜宝审宣宫家容宽宾宿密富寒察寨寺寻寿封射将尉小尔尖尚尤尧尼尾居屏屯山"
    "屿岐岑岔岗岚岛岢岩岫岭岱岳岷岸峄峒峙峡峨峪峰峻崂崃崆崇崖嵊嵩巍川州巢工左巧巨巩巫"
    "巴市布师常干平年广庄庆庐库应底店府度庵康廉廊延建开弋弓张弥强归当彝彦彩彬彭彰征徐"
    "徒得循微德徽心志忠忻怀怒思恒恩恭息恰悟惠感慈戈成戴房手扎托扬扶承投抚拉拐拖招拜拱"
    "指振掇掖措提揭播攀攸改放政故敏敖敦文斗斯新方施旅旌族旗无日旧旬旺昂昆昌明易昔星春"
    "昭晃晋晏晖普景晴暨曙曲曹曼曾月朐朔朗望朝木未末本札杂权李杏村杜杞来杨杭松极林果枝"
    "枞枣柏柔柘柞查柯柱柳柴树栖栗株根格栾桂桃桐桑桓桥桦梁梅梓梦梧梨棉棠棣棱植椒楚楞楼"
    "榆榕槐樊樟模横次歙正步武殷比毕氏民水永氹汀汇汉汕汝江池汤汨汪汶汾沁沂沃沅沈沐沙沚"
    "沛沟沧沭河油治沽沾沿泉泊泌法泗波泰泸泽泾洋洛洞津洪洮洱洲洼流浈浉济浏浑浔浙浚浠浦"
    "浩浪浮海涂涉涞涟涡润涧涪涯涵涿淀淄淅淇淖淞淮深淳清渌渑渝渠渡温渭港游湄湖湘湛湟湾"
    "溆源溧溪滁滋滑滕满滦滨滩滴漠漯漳漾潍潘潜潞潢潭潮潼澄澎澜澧澳濂濉濞濠濮瀍灌灞灯灵"
    "炉炎点烈烟烦烽焉焦煌照熟爱版牌牙牛牟牡牧特犁犍犹独狮猇猗献玄玉王玛环珙珠班珲琅理"
    "琊琼瑞瑶璧瓜瓦瓮瓯甘田申电甸界留略番畴疆疏登白百皇皋皮盂盈益盐监盖盘盟盱直相省眉"
    "眙真睢石矿砀研砚硕硚确碌碑碚碧碱碾磁磐磨磴礼社祁祝神祥票禄禅福禹禺离禾秀秉科秦秭"
    "积称稷稻穆穗穴突立站竞章端竹符等策筠简箐管箭米类精索紫綦繁红级纳细织绍经结绛绥绩"
    "维绵绿缙罕罗罘羊羌美翁翔翠翼耀老考耆耒耿聂聊联肃肇肥胜胡胶脂脱腊腾自至舆舒舞舟船"
    "良色节芒芗芙芜芝芦芬芮花芷苍苏苑苗若英茂范茄茅茌茫茶荃荆草荔荣荥荫荷莆莎莒莘莞莫"
    "莱莲获菏萍萝营萧萨葛葫葵蒗蒙蒲蒸蓉蓝蓟蓥蓬蔚蔡蔺蕉蕲蕴薛藁藏藤虎虞蚌蛟蜀融蠡行街"
    "衡衢袁裕襄西要覃观觉解让讷许诏诸诺调谊谋谟谢谦谯谱谷象豫贝贞贡贤贵费贺贾资赉赛赞"
    "赣赤赫赵起越足路车轮载辉辖辛辰边辽达迁迈迎运进远连迦迪迭逊通遂道遥遵邑邓邕邗邛邡"
    "邢那邮邯邱邳邵邹邺邻郁郊郎郏郑郓郧部郫郭郯郴郸都郾鄂鄄鄞鄠鄢鄯鄱酉酒醴里重野金钟"
    "钢钦钱铁铅铜银锋错锡锦镇镜镶长门间闻闽阁阆阎阜阡防阳阴阿陀陂附陆陇陈陉陕陟陵陶隅"
    "隆随隰雁雄雅集雍雨零雷霄霍霞霸青靖静革鞍韩音韶顶项顺颍额风饶馆首香马驻驿骅高魏鱼"
    "鲁鲅鲤鸠鸡鸣鸭鹤鹰鹿麒麓麟麦麻黄黎黑黔默黟鼎鼓齐龙"
)

def main():
    # 去重并保持顺序
    seen = set()
    chars = []
    for c in CHARS.replace('\n', '').replace(' ', ''):
        if c not in seen:
            seen.add(c)
            chars.append(c)

    print(f"// Auto-generated Chinese font data - {len(chars)} characters")
    print(f"// Characters: {''.join(chars)}")
    print()

    # Try to find a suitable Chinese font
    font_paths = [
        "C:\\Windows\\Fonts\\simsun.ttc",
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\simhei.ttf",
        "C:\\Windows\\Fonts\\STSONG.TTF",
        "C:\\Windows\\Fonts\\simkai.ttf",
        "C:\\Windows\\Fonts\\Deng.ttf",
        "C:\\Windows\\Fonts\\FZSTK.TTF",
    ]

    font = None
    for fp in font_paths:
        if os.path.exists(fp):
            try:
                font = ImageFont.truetype(fp, 14)
                print(f"// Using font: {os.path.basename(fp)}")
                break
            except:
                continue

    if font is None:
        print("ERROR: No Chinese font found!")
        print("Please install a Chinese font or specify the path.")
        return

    # Generate bitmap for each character
    glyphs = []
    for c in chars:
        unicode_val = ord(c)
        # Create a 16x16 image (black bg, white text)
        img = Image.new('1', (16, 16), 0)
        draw = ImageDraw.Draw(img)

        # Center the character in the 16x16 grid
        # For 14pt font on 16x16, draw at (-1, -1) to center
        bbox = font.getbbox(c)
        if bbox:
            text_w = bbox[2] - bbox[0]
            text_h = bbox[3] - bbox[1]
            x = (16 - text_w) // 2 - bbox[0]
            y = (16 - text_h) // 2 - bbox[1]
        else:
            x, y = -1, -1

        draw.text((x, y), c, font=font, fill=1)

        # Convert to hex bytes
        # Format: 2 bytes per row, 16 rows = 32 bytes per char
        # MSB left (Adafruit_GFX default)
        bytes_data = []
        for row in range(16):
            byte_val = 0
            for col in range(8):
                if img.getpixel((col, row)):
                    byte_val |= (0x80 >> col)
            bytes_data.append(byte_val)

            byte_val = 0
            for col in range(8, 16):
                if img.getpixel((col, row)):
                    byte_val |= (0x80 >> (col - 8))
            bytes_data.append(byte_val)

        glyphs.append((c, unicode_val, bytes_data))

    # Output glyph data array
    print("// 16x16 glyph bitmap data (32 bytes per character, MSB-left)")
    print("const uint8_t chineseGlyphData[] PROGMEM = {")
    for idx, (c, code, data) in enumerate(glyphs):
        hex_str = ", ".join(f"0x{b:02X}" for b in data)
        print(f"  // [{idx}] {c} U+{code:04X}")
        print(f"  {hex_str},")
    print("};")
    print()

    # Output font index
    print("// Font index: sorted by Unicode for binary search")
    print("typedef struct {")
    print("  uint16_t code;      // Unicode code point")
    print("  uint16_t glyphIdx;  // Index into chineseGlyphData (×32 bytes)")
    print("} ChineseGlyphIdx;")
    print()

    # Sort by Unicode for binary search
    sorted_glyphs = sorted(glyphs, key=lambda g: g[1])

    print(f"const ChineseGlyphIdx chineseFont[] PROGMEM = {{")
    for idx, (c, code, data) in enumerate(sorted_glyphs):
        orig_idx = next(i for i, g in enumerate(glyphs) if g[1] == code)
        print(f"  {{0x{code:04X}, {orig_idx}}},  // {c}")
    print("};")
    print(f"const int chineseFontCount = {len(sorted_glyphs)};")
    print()

    # Output test
    print("/*")
    print("=== USAGE IN ARDUINO ===")
    print("// Drawing function:")
    print("void drawChineseChar(uint16_t unicode, int x, int y) {")
    print("  // Binary search in chineseFont[], then:")
    print("  int idx = chineseFont[i].glyphIdx;")
    print("  display.drawBitmap(x, y, &chineseGlyphData[idx * 32], 16, 16, SSD1306_WHITE);")
    print("}")
    print()
    print("// UTF-8 decode helper:")
    print("uint16_t utf8ToUnicode(const char* s) {")
    print("  uint8_t c = s[0];")
    print("  if (c < 0x80) return c;")
    print("  if ((c & 0xE0) == 0xC0) return ((c & 0x1F) << 6) | (s[1] & 0x3F);")
    print("  if ((c & 0xF0) == 0xE0) return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);")
    print("  return 0;")
    print("}")
    print("*/")


if __name__ == '__main__':
    main()
