#include "b-em.h"
#include "via.h"
#include "sysvia.h"
#include "keyboard.h"
#include "model.h"

/*
 * The BBC micro keyboard is a fairly standard matrix.  On the Model B,
 * column lines are activated by decoding the output of a 74LS163
 * counter with a 4 to 10 line decoder.  Row lines are pulled up with
 * resistors and fed both to an eight input NAND gate to generate an
 * interrupt and to a 74LS251 multiplexer which allows the row lines
 * to be read.
 *
 * The Master adds an extra three columns to the matrix to handle the
 * numeric keymap.  In the diagram below columns 0x00-0x09 are common
 * to the Model B and the Master while 0x0a-0x0c are Master-only.
 *
 * The diagram in the original version of The Advanced User Guide is
 * slighly misleading because the bits as seen by the 74LS251 do not
 * match the rows shown in the diagram.  From a software perspective
 * the keybaord looks like this:
 *
 *       0x00      0x01  0x02  0x03 0x04 0x05 0x06 0x07 0x08 0x09    0x0a   0x0b   0x0c
 * 0x00  Shift     Ctrl  <------- starup up DIP swicthes ------->
 * 0x10  Q         3     4     5    f4   8    f7   =-   ~^   Left    KP 6   KP 7
 * 0x20  f0        W     E     T    7    I    9    0    £    Down    KP 8   KP 9
 * 0x30  1         2     D     R    6    U    O    P    [{   Up      KP +   KP -   KP Return
 * 0x40  CapsLck   A     X     F    Y    J    K    @    :*   Return  KP /   KP Del KP .
 * 0x50  ShiftLck  S     C     G    H    N    L    ;+   ]}   Delete  KP #   KP *   KP ,
 * 0x60  Tab       Z     SPC   V    B    M    <,   >.   /?   Copy    KP 0   KP 1   KP 3
 * 0x70  ESC       f1    f2    f3   f5   f6   f8   f9   \    Right   KP 4   KP 4   KP 2
 *
*/

static const uint8_t allegro2bbc[ALLEGRO_KEY_MAX] =
{
    0xaa,   // 0
    0x41,   // 1    ALLEGRO_KEY_A
    0x64,   // 2    ALLEGRO_KEY_B
    0x52,   // 3    ALLEGRO_KEY_C
    0x32,   // 4    ALLEGRO_KEY_D
    0x22,   // 5    ALLEGRO_KEY_E
    0x43,   // 6    ALLEGRO_KEY_F
    0x53,   // 7    ALLEGRO_KEY_G
    0x54,   // 8    ALLEGRO_KEY_H
    0x25,   // 9    ALLEGRO_KEY_I
    0x45,   // 10   ALLEGRO_KEY_J
    0x46,   // 11   ALLEGRO_KEY_K
    0x56,   // 12   ALLEGRO_KEY_L
    0x65,   // 13   ALLEGRO_KEY_M
    0x55,   // 14   ALLEGRO_KEY_N
    0x36,   // 15   ALLEGRO_KEY_O
    0x37,   // 16   ALLEGRO_KEY_P
    0x10,   // 17   ALLEGRO_KEY_Q
    0x33,   // 18   ALLEGRO_KEY_R
    0x51,   // 19   ALLEGRO_KEY_S
    0x23,   // 20   ALLEGRO_KEY_T
    0x35,   // 21   ALLEGRO_KEY_U
    0x63,   // 22   ALLEGRO_KEY_V
    0x21,   // 23   ALLEGRO_KEY_W
    0x42,   // 24   ALLEGRO_KEY_X
    0x44,   // 25   ALLEGRO_KEY_Y
    0x61,   // 26   ALLEGRO_KEY_Z
    0x27,   // 27   ALLEGRO_KEY_0
    0x30,   // 28   ALLEGRO_KEY_1
    0x31,   // 29   ALLEGRO_KEY_2
    0x11,   // 30   ALLEGRO_KEY_3
    0x12,   // 31   ALLEGRO_KEY_4
    0x13,   // 32   ALLEGRO_KEY_5
    0x34,   // 33   ALLEGRO_KEY_6
    0x24,   // 34   ALLEGRO_KEY_7
    0x15,   // 35   ALLEGRO_KEY_8
    0x26,   // 36   ALLEGRO_KEY_9
    0x6a,   // 37   ALLEGRO_KEY_PAD_0
    0x6b,   // 38   ALLEGRO_KEY_PAD_1
    0x7c,   // 39   ALLEGRO_KEY_PAD_2
    0x6c,   // 40   ALLEGRO_KEY_PAD_3
    0x7a,   // 41   ALLEGRO_KEY_PAD_4
    0x7b,   // 42   ALLEGRO_KEY_PAD_5
    0x1a,   // 43   ALLEGRO_KEY_PAD_6
    0x1b,   // 44   ALLEGRO_KEY_PAD_7
    0x2a,   // 45   ALLEGRO_KEY_PAD_8
    0x2b,   // 46   ALLEGRO_KEY_PAD_9
    0x20,   // 56   ALLEGRO_KEY_F1
    0x71,   // 47   ALLEGRO_KEY_F2
    0x72,   // 48   ALLEGRO_KEY_F3
    0x73,   // 49   ALLEGRO_KEY_F4
    0x14,   // 50   ALLEGRO_KEY_F5
    0x74,   // 51   ALLEGRO_KEY_F6
    0x75,   // 52   ALLEGRO_KEY_F7
    0x16,   // 53   ALLEGRO_KEY_F8
    0x76,   // 54   ALLEGRO_KEY_F9
    0x77,   // 55   ALLEGRO_KEY_F10
    0x28,   // 57   ALLEGRO_KEY_F11
    0xaa,   // 58   ALLEGRO_KEY_F12
    0x70,   // 59   ALLEGRO_KEY_ESCAPE
    0x28,   // 60   ALLEGRO_KEY_TILDE
    0x17,   // 61   ALLEGRO_KEY_MINUS
    0x18,   // 62   ALLEGRO_KEY_EQUALS
    0x59,   // 63   ALLEGRO_KEY_BACKSPACE
    0x60,   // 64   ALLEGRO_KEY_TAB
    0x47,   // 65   ALLEGRO_KEY_OPENBRACE
    0x38,   // 66   ALLEGRO_KEY_CLOSEBRACE
    0x49,   // 67   ALLEGRO_KEY_ENTER
    0x57,   // 68   ALLEGRO_KEY_SEMICOLON
    0x48,   // 69   ALLEGRO_KEY_QUOTE
    0x58,   // 70   ALLEGRO_KEY_BACKSLASH
    0x78,   // 71   ALLEGRO_KEY_BACKSLASH2
    0x66,   // 72   ALLEGRO_KEY_COMMA
    0x67,   // 73   ALLEGRO_KEY_FULLSTOP
    0x68,   // 74   ALLEGRO_KEY_SLASH
    0x62,   // 75   ALLEGRO_KEY_SPACE
    0xaa,   // 76   ALLEGRO_KEY_INSERT
    0x59,   // 77   ALLEGRO_KEY_DELETE
    0x5c,   // 78   ALLEGRO_KEY_HOME
    0x69,   // 79   ALLEGRO_KEY_END
    0xaa,   // 80   ALLEGRO_KEY_PGUP
    0x4c,   // 81   ALLEGRO_KEY_PGDN
    0x19,   // 82   ALLEGRO_KEY_LEFT
    0x79,   // 83   ALLEGRO_KEY_RIGHT
    0x39,   // 84   ALLEGRO_KEY_UP
    0x29,   // 85   ALLEGRO_KEY_DOWN
    0x4a,   // 86   ALLEGRO_KEY_PAD_SLASH
    0x5b,   // 87   ALLEGRO_KEY_PAD_ASTERISK
    0x3b,   // 88   ALLEGRO_KEY_PAD_MINUS
    0x3a,   // 89   ALLEGRO_KEY_PAD_PLUS
    0x59,   // 90   ALLEGRO_KEY_PAD_DELETE
    0x3c,   // 91   ALLEGRO_KEY_PAD_ENTER
    0x4c,   // 92   ALLEGRO_KEY_PRINTSCREEN
    0xaa,   // 93   ALLEGRO_KEY_PAUSE
    0x4c,   // 94   ALLEGRO_KEY_ABNT_C1
    0xaa,   // 95   ALLEGRO_KEY_YEN
    0xaa,   // 96   ALLEGRO_KEY_KANA
    0xaa,   // 97   ALLEGRO_KEY_CONVERT
    0xaa,   // 98   ALLEGRO_KEY_NOCONVERT
    0x00,   // 99   ALLEGRO_KEY_AT
    0x00,   // 100  ALLEGRO_KEY_CIRCUMFLEX
    0x01,   // 101  ALLEGRO_KEY_COLON2
    0xaa,   // 102  ALLEGRO_KEY_KANJI
    0x50,   // 103  ALLEGRO_KEY_PAD_EQUALS
    0xaa,   // 104  ALLEGRO_KEY_BACKQUOTE
    0x57,   // 105  ALLEGRO_KEY_SEMICOLON2
    0xaa,   // 106  ALLEGRO_KEY_COMMAND
    0x50,   // 107  ALLEGRO_KEY_BACK
    0xaa,   // 108  ALLEGRO_KEY_VOLUME_UP
    0xaa,   // 109  ALLEGRO_KEY_VOLUME_DOWN
    0xaa,   // 110  ALLEGRO_KEY_SEARCH
    0xaa,   // 111  ALLEGRO_KEY_DPAD_CENTER
    0xaa,   // 112  ALLEGRO_KEY_BUTTON_X
    0xaa,   // 113  ALLEGRO_KEY_BUTTON_Y
    0xaa,   // 114  ALLEGRO_KEY_DPAD_UP
    0xaa,   // 115  ALLEGRO_KEY_DPAD_DOWN
    0xaa,   // 116  ALLEGRO_KEY_DPAD_LEFT
    0xaa,   // 117  ALLEGRO_KEY_DPAD_RIGHT
    0xaa,   // 118  ALLEGRO_KEY_SELECT
    0xaa,   // 119  ALLEGRO_KEY_START
    0xaa,   // 120  ALLEGRO_KEY_BUTTON_L1
    0xaa,   // 121  ALLEGRO_KEY_BUTTON_R1
    0xaa,   // 122  ALLEGRO_KEY_BUTTON_L2
    0xaa,   // 123  ALLEGRO_KEY_BUTTON_R2
    0xaa,   // 124  ALLEGRO_KEY_BUTTON_A
    0xaa,   // 125  ALLEGRO_KEY_BUTTON_B
    0xaa,   // 126  ALLEGRO_KEY_THUMBL
    0xaa,   // 127  ALLEGRO_KEY_THUMBR
    0xaa,   // 128
    0xaa,   // 129
    0xaa,   // 130
    0xaa,   // 131
    0xaa,   // 132
    0xaa,   // 133
    0xaa,   // 134
    0xaa,   // 135
    0xaa,   // 136
    0xaa,   // 137
    0xaa,   // 138
    0xaa,   // 139
    0xaa,   // 140
    0xaa,   // 141
    0xaa,   // 142
    0xaa,   // 143
    0xaa,   // 144
    0xaa,   // 145
    0xaa,   // 146
    0xaa,   // 147
    0xaa,   // 148
    0xaa,   // 149
    0xaa,   // 150
    0xaa,   // 151
    0xaa,   // 152
    0xaa,   // 153
    0xaa,   // 154
    0xaa,   // 155
    0xaa,   // 156
    0xaa,   // 157
    0xaa,   // 158
    0xaa,   // 159
    0xaa,   // 160
    0xaa,   // 161
    0xaa,   // 162
    0xaa,   // 163
    0xaa,   // 164
    0xaa,   // 165
    0xaa,   // 166
    0xaa,   // 167
    0xaa,   // 168
    0xaa,   // 169
    0xaa,   // 170
    0xaa,   // 171
    0xaa,   // 172
    0xaa,   // 173
    0xaa,   // 174
    0xaa,   // 175
    0xaa,   // 176
    0xaa,   // 177
    0xaa,   // 178
    0xaa,   // 179
    0xaa,   // 180
    0xaa,   // 181
    0xaa,   // 182
    0xaa,   // 183
    0xaa,   // 184
    0xaa,   // 185
    0xaa,   // 186
    0xaa,   // 187
    0xaa,   // 188
    0xaa,   // 189
    0xaa,   // 190
    0xaa,   // 191
    0xaa,   // 192
    0xaa,   // 193
    0xaa,   // 194
    0xaa,   // 195
    0xaa,   // 196
    0xaa,   // 197
    0xaa,   // 198
    0xaa,   // 199
    0xaa,   // 200
    0xaa,   // 201
    0xaa,   // 202
    0xaa,   // 203
    0xaa,   // 204
    0xaa,   // 205
    0xaa,   // 206
    0xaa,   // 207
    0xaa,   // 208
    0xaa,   // 209
    0xaa,   // 210
    0xaa,   // 211
    0xaa,   // 212
    0xaa,   // 213
    0xaa,   // 214
    0x00,   // 215  ALLEGRO_KEY_LSHIFT
    0x00,   // 216  ALLEGRO_KEY_RSHIFT
    0x01,   // 217  ALLEGRO_KEY_LCTRL
    0x01,   // 218  ALLEGRO_KEY_RCTRL
    0xaa,   // 219  ALLEGRO_KEY_ALT
    0xaa,   // 220  ALLEGRO_KEY_ALTGR
    0xaa,   // 221  ALLEGRO_KEY_LWIN
    0xaa,   // 222  ALLEGRO_KEY_RWIN
    0xaa,   // 223  ALLEGRO_KEY_MENU
    0xaa,   // 224  ALLEGRO_KEY_SCROLLLOCK
    0x71,   // 225  ALLEGRO_KEY_NUMLOCK
    0x40,   // 226  ALLEGRO_KEY_CAPSLOCK
};

int keylookup[ALLEGRO_KEY_MAX];
bool keyas  = false;
bool keypad = false;

static int keycol, keyrow;
static int bbckey[16][16];

void key_clear(void)
{
    int c, r;
    for (c = 0; c < 16; c++)
        for (r = 0; r < 16; r++)
            bbckey[c][r] = 0;
    sysvia_set_ca2(0);
}

static void key_update()
{
    int maxcol = (MASTER) ? 13 : 10;
    if (IC32 & 8) {
        /* autoscan mode */
        for (int col = 0; col < maxcol; col++) {
            for (int row = 1; row < 8; row++) {
                if (bbckey[col][row]) {
                    sysvia_set_ca2(1);
                    return;
                }
            }
        }
    }
    else {
        /* scan specific key mode */
        if (keycol < maxcol) {
            for (int row = 1; row < 8; row++) {
                if (bbckey[keycol][row]) {
                    sysvia_set_ca2(1);
                    return;
                }
            }
        }
    }
    sysvia_set_ca2(0);
}

static const int map_keypad[] = {
    /* ALLEGRO_KEY_PAD_0 */ ALLEGRO_KEY_INSERT,
    /* ALLEGRO_KEY_PAD_1 */ ALLEGRO_KEY_END,
    /* ALLEGRO_KEY_PAD_2 */ ALLEGRO_KEY_DOWN,
    /* ALLEGRO_KEY_PAD_3 */ ALLEGRO_KEY_PGDN,
    /* ALLEGRO_KEY_PAD_4 */ ALLEGRO_KEY_LEFT,
    /* ALLEGRO_KEY_PAD_5 */ ALLEGRO_KEY_PAD_5,
    /* ALLEGRO_KEY_PAD_6 */ ALLEGRO_KEY_RIGHT,
    /* ALLEGRO_KEY_PAD_7 */ ALLEGRO_KEY_HOME,
    /* ALLEGRO_KEY_PAD_8 */ ALLEGRO_KEY_UP,
    /* ALLEGRO_KEY_PAD_9 */ ALLEGRO_KEY_PGUP
};

int key_map(ALLEGRO_EVENT *event)
{
    int code = event->keyboard.keycode;
    if (code < ALLEGRO_KEY_MAX) {
        if (code == ALLEGRO_KEY_A) {
            if (keyas)
                code = ALLEGRO_KEY_CAPSLOCK;
        }
        else if (code >= ALLEGRO_KEY_PAD_0 && code <= ALLEGRO_KEY_PAD_9 && keypad && !(event->keyboard.modifiers & ALLEGRO_KEYMOD_NUMLOCK))
                code = map_keypad[code-ALLEGRO_KEY_PAD_0];
        code = keylookup[code];
    }
    log_debug("keyboard: unichar=%d, mapping %d to %d", event->keyboard.unichar, event->keyboard.keycode, code);
    return code;
}

static void set_key(int code, int state)
{
    unsigned vkey;

    vkey = allegro2bbc[code];
    log_debug("keyboard: code=%d, vkey=%02X", code, vkey);
    if (vkey != 0xaa) {
        bbckey[vkey & 15][vkey >> 4] = state;
        key_update();
    }
}

void key_down(int code)
{
    set_key(code, 1);
}

void key_up(int code)
{
    set_key(code, 0);
}

void key_scan(int row, int col) {
    keyrow = row;
    keycol = col;
    key_update();
}

bool key_is_down(void) {
    if (keyrow == 0 && keycol >= 2 && keycol <= 9)
        return kbdips & (1 << (9 - keycol));
    else
        return bbckey[keycol][keyrow];
}

bool key_any_down(void)
{
    for (int c = 0; c < 16; c++)
        for (int r = 1; r < 16; r++)
            if (bbckey[c][r])
                return true;
    return false;
}

bool key_code_down(int code)
{
    if (code < ALLEGRO_KEY_MAX) {
        code = allegro2bbc[code];
        return bbckey[code & 0x0f][code >> 4];
    }
    return false;
}
