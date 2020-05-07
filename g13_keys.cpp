/* This file contains code for managing keys an profiles
 *
 */

// TODO: check out why g13.h needs to be included before libevdev
// clang-format off
#include "g13.hpp"
#include "g13_device.hpp"
#include <libevdev-1.0/libevdev/libevdev.h>
// clang-format on

namespace G13 {

    // TODO: theese are just hanging in the global place here

    static void init_symbols() {
        // A quick fix for not using initialized global static consts
        /*! sequence containing the
         * G13 keys.  The order is very specific, with the position of each
         * item corresponding to a specific bit in the G13's USB message
         * format.  Do NOT remove or insert items in this list.
         */
        // clang-format off
        G13_KEY_STRINGS = {  // formerly G13_KEY_SEQ
                /* byte 3 */
                "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8",
                /* byte 4 */
                "G9", "G10", "G11", "G12", "G13", "G14", "G15", "G16",
                /* byte 5 */
                "G17", "G18", "G19", "G20", "G21", "G22", "UNDEF1", "LIGHT_STATE",
                /* byte 6 */
                "BD", "L1", "L2", "L3", "L4", "M1", "M2", "M3",
                /* byte 7 */
                "MR", "LEFT", "DOWN", "TOP", "UNDEF3", "LIGHT", "LIGHT2", "MISC_TOGGLE",
        };
        // clang-format on

        /*! sequence containing the
         * G13 keys that shouldn't be tested input.  These aren't actually keys,
         * but they are in the bitmap defined by G13_KEY_SEQ.
         */
        // formerly G13_NONPARSED_KEY_SEQ
        G13_NONPARSED_KEYS = {"UNDEF1", "LIGHT_STATE", "UNDEF3", "LIGHT",
                              "LIGHT2", "UNDEF3", "MISC_TOGGLE"};


        /*! sequence containing the
         * names of keyboard keys we can send through binding actions.
         * These correspond to KEY_xxx value definitions in <linux/input.h>,
         * i.e. ESC is KEY_ESC, 1 is KEY_1, etc.
         */
        // formerly KB_INPUT_KEY_SEQ
        // clang-format off
        G13_SYMBOLS = {
                "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "MINUS", "EQUAL", "BACKSPACE",
                "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O",
                "P", "LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL", "RIGHTCTRL", "A", "S", "D", "F",
                "G", "H", "J", "K", "L", "SEMICOLON",
                "APOSTROPHE", "GRAVE", "LEFTSHIFT", "BACKSLASH", "Z", "X", "C", "V", "B", "N", "M",
                "COMMA", "DOT", "SLASH", "RIGHTSHIFT",
                "KPASTERISK", "LEFTALT", "RIGHTALT", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5",
                "F6", "F7", "F8", "F9", "F10", "F11", "F12",
                "NUMLOCK", "SCROLLLOCK", "KP7", "KP8", "KP9", "KPMINUS", "KP4", "KP5", "KP6", "KPPLUS",
                "KP1", "KP2", "KP3", "KP0", "KPDOT",
                "KPSLASH", "LEFT", "RIGHT", "UP", "DOWN", "PAGEUP", "PAGEDOWN", "HOME", "END", "INSERT",
                "DELETE", "F13", "F14", "F15", "F16",
                "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "NEXTSONG", "PLAYPAUSE",
                "PREVIOUSSONG"
        };
        // clang-format on

        // formerly M_INPUT_BTN_SEQ
        /*! m_INPUT_BTN_SEQ is a Boost Preprocessor sequence containing the
         * names of button events we can send through binding actions.
         * These correspond to BTN_xxx value definitions in <linux/input.h>,
         * i.e. LEFT is BTN_LEFT, RIGHT is BTN_RIGHT, etc.
         *
         * The binding names have prefix M to avoid naming conflicts.
         * e.g. LEFT keyboard button and LEFT mouse button
         * i.e. LEFT mouse button is named MLEFT, MIDDLE mouse button is MMIDDLE
         */

        // clang-format off
        G13_BTN_SEQ = { "LEFT", "RIGHT", "MIDDLE", "SIDE", "EXTRA" };
        // clang-format on
    }

// Decided against using an enum too, I see no point
// clang-format off
//enum {
//    /* byte 3 */
//     G1, G2, G3, G4, G5, G6, G7, G8,
//    /* byte 4 */
//     G9,G10,G11,G12,G13,G14,G15,G16,
//    /* byte 5 */
//    G17,G18,G19,G20,G21,G22,UNDEF1,LIGHT_STATE,
//    /* byte 6 */                                                \
//    BD,L1,L2,L3,L4,M1,M2,M3,
//    /* byte 7 */
//    MR,LEFT,DOWN,TOP,UNDEF3,LIGHT,LIGHT2,MISC_TOGGLE,
//} G13_KEYS;
// clang-format on

void G13_Profile::_init_keys() {
    // create a G13_Key entry for every key in G13_KEY_SEQ
    int key_index = 0;
    for (auto& symbol : G13_KEY_STRINGS) {
        _keys.emplace_back(G13_Key(*this, symbol, key_index));
        key_index++;
    }
    assert(_keys.size() == G13_NUM_KEYS);

    // now disable testing for keys in G13_NONPARSED_KEY_SEQ
    for (auto& symbol : G13_NONPARSED_KEYS) {
        G13_Key* key = find_key(symbol);
        key->_should_parse = false;
    }
}

void G13_Key::dump(std::ostream& o) const {
    o << manager().find_g13_key_name(index()) << "(" << index() << ") : ";
    if (action()) {
        action()->dump(o);
    } else {
        o << "(no action)";
    }
}

void G13_Profile::dump(std::ostream& o) const {
    o << "Profile " << repr(name()) << std::endl;
    // BOOST_FOREACH (const G13_Key& key, _keys) {
    for (auto& key : _keys) {
        if (key.action()) {
            o << "   ";
            key.dump(o);
            o << std::endl;
        }
    }
}

void G13_Profile::parse_keys(unsigned char* buf) {
    buf += 3;
    for (auto& _key : _keys) {
        if (_key._should_parse) {
            _key.parse_key(buf, &_keypad);
        }
    }
}

G13_Key* G13_Profile::find_key(const std::string& keyname) {
    auto key = _keypad.manager().find_g13_key_value(keyname);
    if (key >= 0 && key < _keys.size()) {
        return &_keys[key];
    }
    return nullptr;
}

// *************************************************************************

void G13_Key::parse_key(const unsigned char* byte, G13_Device* g13) {
    bool key_is_down = byte[_index.offset] & _index.mask;
    auto key_state_changed = g13->update(_index.index, key_is_down);

    if (key_state_changed && _action) {
        _action->act(*g13, key_is_down);
    }
}

// *************************************************************************

void G13_Manager::init_keynames() {
    init_symbols();
    int key_index = 0;

    // setup maps to let us convert between strings and G13 key names
    for (auto& name : G13_KEY_STRINGS) {
        g13_key_to_name[key_index] = name;
        g13_name_to_key[name] = key_index;
        G13_DBG("mapping G13 " << name << " = " << key_index);
        key_index++;
    }

    // setup maps to let us convert between strings and linux key names
    for (auto& symbol : G13_SYMBOLS) {
        auto keyname = std::string("KEY_" + symbol);

        int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
        if (code < 0) {
            G13_ERR("No input event code found for " << keyname);
        } else {
            // TODO: this seems to map ok but the result is off
            // assert(keyname.compare(libevdev_event_code_get_name(EV_KEY,code)) == 0);
            // linux/input-event-codes.h

            input_key_to_name[code] = symbol;
            input_name_to_key[symbol] = code;
            G13_DBG("mapping " << symbol << " " << keyname << "=" << code);
        }
    }

    // setup maps to let us convert between strings and linux button names
    for (auto& symbol : G13_BTN_SEQ) {
        auto name = std::string("M" + symbol);
        auto keyname = std::string("BTN_" + symbol);
        int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
        if (code < 0) {
            G13_ERR("No input event code found for " << keyname);
        } else {
            input_key_to_name[code] = name;
            input_name_to_key[name] = code;
            G13_DBG("mapping " << name << " " << keyname << "=" << code);
        }
    }
}

LINUX_KEY_VALUE
G13_Manager::find_g13_key_value(const std::string& keyname) const {
    auto i = g13_name_to_key.find(keyname);
    if (i == g13_name_to_key.end()) {
        return BAD_KEY_VALUE;
    }
    return i->second;
}

LINUX_KEY_VALUE
G13_Manager::find_input_key_value(const std::string& keyname) const {
    // if there is a KEY_ prefix, strip it off
    if (!strncmp(keyname.c_str(), "KEY_", 4)) {
        return find_input_key_value(keyname.c_str() + 4);
    }

    auto i = input_name_to_key.find(keyname);
    if (i == input_name_to_key.end()) {
        return BAD_KEY_VALUE;
    }
    return i->second;
}

std::string G13_Manager::find_input_key_name(LINUX_KEY_VALUE v) const {
    try {
        return find_or_throw(input_key_to_name, v);
    } catch (...) {
        return "(unknown linux key)";
    }
}

std::string G13_Manager::find_g13_key_name(G13_KEY_INDEX v) const {
    try {
        return find_or_throw(g13_key_to_name, v);
    } catch (...) {
        return "(unknown G13 key)";
    }
}

void G13_Manager::display_keys() {
    typedef std::map<std::string, int> mapType;
    G13_OUT("Known keys on G13:");
    G13_OUT(Helper::map_keys_out(g13_name_to_key));

    G13_OUT("Known keys to map to:");
    G13_OUT(Helper::map_keys_out(input_name_to_key));
}

}  // namespace G13
