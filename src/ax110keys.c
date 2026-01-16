#include <stdbool.h>
#include <stdint.h>

#include "zephyr/usb/class/hid.h"

static bool blueAlt = false;

uint8_t input_to_hid(uint16_t code, int32_t value)
{
    int hid_code = 0;
    switch (code)
    {
    case 0x2:
        blueAlt = (bool)value;
        hid_code = 0;
        break;
    case 0x207: hid_code = HID_KEY_TAB;
        break; //L IND
    case 0x702: hid_code = HID_KEY_1;
        break;
    case 0x602: hid_code = HID_KEY_2;
        break;
    case 0x703: hid_code = HID_KEY_3;
        break;
    case 0x603: hid_code = HID_KEY_4;
        break;
    case 0x705: hid_code = HID_KEY_5;
        break;
    case 0x605: hid_code = HID_KEY_6;
        break;
    case 0x704: hid_code = HID_KEY_7;
        break;
    case 0x604: hid_code = HID_KEY_8;
        break;
    case 0x707: hid_code = HID_KEY_9;
        break;
    case 0x607: hid_code = HID_KEY_0;
        break;
    case 0x706: hid_code = HID_KEY_SLASH;
        break;
    case 0x606: hid_code = HID_KEY_GRAVE;
        break;
    case 0x103: hid_code = HID_KEY_PAGEUP;
        break; //RELOC
    case 0x102: hid_code = HID_KEY_PAGEDOWN;
        break; //INDEX
    case 0x501: hid_code = HID_KEY_Q;
        break;
    case 0x301: hid_code = HID_KEY_W;
        break;
    case 0x502: hid_code = HID_KEY_E;
        break;
    case 0x302: hid_code = HID_KEY_R;
        break;
    case 0x503: hid_code = HID_KEY_T;
        break;
    case 0x303: hid_code = HID_KEY_Y;
        break;
    case 0x505: hid_code = HID_KEY_U;
        break;
    case 0x305: hid_code = HID_KEY_I;
        break;
    case 0x504: hid_code = HID_KEY_O;
        break;
    case 0x304: hid_code = HID_KEY_P;
        break;
    case 0x500: hid_code = HID_KEY_LEFTBRACE;
        break; //Ring A(Swedish OO)
    case 0x300: hid_code = HID_KEY_RIGHTBRACE;
        break; // U umlaut
    case 0x105: hid_code = HID_KEY_BACKSPACE;
        break;
    case 0x01: hid_code = HID_KEY_CAPSLOCK;
        break;
    case 0x201: hid_code = HID_KEY_A;
        break;
    case 0x404: hid_code = HID_KEY_S;
        break;
    case 0x204: hid_code = HID_KEY_D;
        break;
    case 0x402: hid_code = HID_KEY_F;
        break;
    case 0x202: hid_code = HID_KEY_G;
        break;
    case 0x403: hid_code = HID_KEY_H;
        break;
    case 0x203: hid_code = HID_KEY_J;
        break;
    case 0x405: hid_code = HID_KEY_K;
        break;
    case 0x205: hid_code = HID_KEY_L;
        break;
    case 0x400: hid_code = HID_KEY_SEMICOLON;
        break; // O umlaut
    case 0x200: hid_code = HID_KEY_APOSTROPHE;
        break; //A umlaut
    case 0x601: hid_code = HID_KEY_BACKSLASH;
        break; //*
    case 0x101: hid_code = HID_KEY_ENTER;
        break;
    case 0x401: hid_code = HID_KEY_Z;
        break;
    case 0x406: hid_code = HID_KEY_X;
        break;
    case 0x407: hid_code = HID_KEY_C;
        break;
    case 0x507: hid_code = HID_KEY_V;
        break;
    case 0x307: hid_code = HID_KEY_B;
        break;
    case 0x506: hid_code = HID_KEY_N;
        break;
    case 0x306: hid_code = HID_KEY_M;
        break;
    case 0x700: hid_code = HID_KEY_COMMA;
        break;
    case 0x600: hid_code = HID_KEY_DOT;
        break;
    case 0x701: hid_code = HID_KEY_MINUS;
        break;
    case 0x00: hid_code = HID_KBD_MODIFIER_RIGHT_SHIFT;
        break; //Both shifts
    case 0x107: hid_code = HID_KBD_MODIFIER_LEFT_CTRL;
        break; //Green CODE
    case 0x104: hid_code = HID_KBD_MODIFIER_LEFT_ALT;
        break; // WORD OUT
    case 0x100: hid_code = HID_KEY_SPACE;
        break;
    case 0x106: hid_code = HID_KEY_DELETE;
        break; // delete or R ALT?
    default: hid_code = 0;
    }
    if (blueAlt)
    {
        switch (hid_code)
        {
        case HID_KEY_TAB: hid_code = HID_KEY_ESC;//L IND
            break;
        case HID_KEY_1: hid_code = HID_KEY_F1;
            break;
        case HID_KEY_2: hid_code = HID_KEY_F2;
            break;
        case HID_KEY_3: hid_code = HID_KEY_F3;
            break;
        case HID_KEY_4: hid_code = HID_KEY_F4;
            break;
        case HID_KEY_5: hid_code = HID_KEY_F5;
            break;
        case HID_KEY_6: hid_code = HID_KEY_F6;
            break;
        case HID_KEY_7: hid_code = HID_KEY_F7;
            break;
        case HID_KEY_8: hid_code = HID_KEY_F8;
            break;
        case HID_KEY_9: hid_code = HID_KEY_F9;
            break;
        case HID_KEY_0: hid_code = HID_KEY_F10;
            break;
        case HID_KEY_SLASH: hid_code = HID_KEY_F11;
            break;
        case HID_KEY_GRAVE: hid_code = HID_KEY_F12;
            break;
        case HID_KEY_PAGEUP: hid_code = HID_KEY_HOME;//RELOC
            break;
        case HID_KEY_PAGEDOWN: hid_code = HID_KEY_END;//INDEX
            break;
        case HID_KEY_W: hid_code = HID_KEY_UP;
            break;
        case HID_KEY_A: hid_code = HID_KEY_LEFT;
            break;
        case HID_KEY_S: hid_code = HID_KEY_DOWN;
            break;
        case HID_KEY_D: hid_code = HID_KEY_RIGHT;
            break;
        }
    }
    return hid_code;
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
