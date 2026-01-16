#include <stdbool.h>
#include <stdint.h>

#include "zephyr/usb/class/hid.h"

uint8_t input_to_hid(uint16_t code)
{
    switch (code)
    {
    //case 0x2: return HID_KEY_ALT; must be modifying other buttons
    case 0x207: return HID_KEY_TAB; //L IND
    case 0x702: return HID_KEY_1;
    case 0x602: return HID_KEY_2;
    case 0x703: return HID_KEY_3;
    case 0x603: return HID_KEY_4;
    case 0x705: return HID_KEY_5;
    case 0x605: return HID_KEY_6;
    case 0x704: return HID_KEY_7;
    case 0x604: return HID_KEY_8;
    case 0x707: return HID_KEY_9;
    case 0x607: return HID_KEY_0;
    case 0x706: return HID_KEY_SLASH;
    case 0x606: return HID_KEY_GRAVE;
    //	case 0x103: return HID_KEY_RELOC
    //	case 0x102: return HID_KEY_INDEX
    case 0x501: return HID_KEY_Q;
    case 0x301: return HID_KEY_W;
    case 0x502: return HID_KEY_E;
    case 0x302: return HID_KEY_R;
    case 0x503: return HID_KEY_T;
    case 0x303: return HID_KEY_Y;
    case 0x505: return HID_KEY_U;
    case 0x305: return HID_KEY_I;
    case 0x504: return HID_KEY_O;
    case 0x304: return HID_KEY_P;
    case 0x500: return HID_KEY_LEFTBRACE; //Ring A(Swedish OO)
    case 0x300: return HID_KEY_RIGHTBRACE; // U umlaut
    case 0x105: return HID_KEY_BACKSPACE;
    case 0x01: return HID_KEY_CAPSLOCK;
    case 0x201: return HID_KEY_A;
    case 0x404: return HID_KEY_S;
    case 0x204: return HID_KEY_D;
    case 0x402: return HID_KEY_F;
    case 0x202: return HID_KEY_G;
    case 0x403: return HID_KEY_H;
    case 0x203: return HID_KEY_J;
    case 0x405: return HID_KEY_K;
    case 0x205: return HID_KEY_L;
    case 0x400: return HID_KEY_SEMICOLON; // O umlaut
    case 0x200: return HID_KEY_APOSTROPHE; //A umlaut
    case 0x601: return HID_KEY_BACKSLASH; //*
    case 0x101: return HID_KEY_ENTER;
    case 0x401: return HID_KEY_Z;
    case 0x406: return HID_KEY_X;
    case 0x407: return HID_KEY_C;
    case 0x507: return HID_KEY_V;
    case 0x307: return HID_KEY_B;
    case 0x506: return HID_KEY_N;
    case 0x306: return HID_KEY_M;
    case 0x700: return HID_KEY_COMMA;
    case 0x600: return HID_KEY_DOT;
    case 0x701: return HID_KEY_MINUS;
    case 0x00: return HID_KBD_MODIFIER_RIGHT_SHIFT; //Both shifts
    case 0x107: return HID_KBD_MODIFIER_LEFT_CTRL; //Green CODE
    case 0x104: return HID_KBD_MODIFIER_RIGHT_ALT; // WORD OUT
    case 0x100: return HID_KEY_SPACE;
    case 0x106: return HID_KEY_DELETE; // delete or R ALT?
    default: return 0;
    }
}

bool is_modifier(uint16_t code)
{
    switch (code)
    {
    //case 0x2: return HID_KEY_ALT; must be modifying other buttons
    //	case 0x103: return HID_KEY_RELOC
    //	case 0x102: return HID_KEY_INDEX
    //    case 0x106:// delete or R ALT?
    case 0x00: //Both shifts
    case 0x107: //Green CODE
    case 0x104: // WORD OUT
        return true;
    default: return false;
    }
}
