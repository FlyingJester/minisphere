#include "minisphere.h"
#include "api.h"
#include "input.h"

#define MAX_JOYSTICKS    4
#define MAX_JOY_BUTTONS  32

struct key_queue
{
	int num_keys;
	int keys[255];
};

enum mouse_button
{
	MOUSE_BUTTON_LEFT = 1,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE
};

enum mouse_wheel_event
{
	MOUSE_WHEEL_UP,
	MOUSE_WHEEL_DOWN
};

static duk_ret_t js_AreKeysLeft             (duk_context* ctx);
static duk_ret_t js_IsAnyKeyPressed         (duk_context* ctx);
static duk_ret_t js_IsJoystickButtonPressed (duk_context* ctx);
static duk_ret_t js_IsKeyPressed            (duk_context* ctx);
static duk_ret_t js_IsMouseButtonPressed    (duk_context* ctx);
static duk_ret_t js_GetJoystickAxis         (duk_context* ctx);
static duk_ret_t js_GetKey                  (duk_context* ctx);
static duk_ret_t js_GetKeyString            (duk_context* ctx);
static duk_ret_t js_GetNumMouseWheelEvents  (duk_context* ctx);
static duk_ret_t js_GetMouseWheelEvent      (duk_context* ctx);
static duk_ret_t js_GetMouseX               (duk_context* ctx);
static duk_ret_t js_GetMouseY               (duk_context* ctx);
static duk_ret_t js_GetNumJoysticks         (duk_context* ctx);
static duk_ret_t js_GetNumJoystickAxes      (duk_context* ctx);
static duk_ret_t js_GetNumJoystickButtons   (duk_context* ctx);
static duk_ret_t js_GetNumMouseWheelEvents  (duk_context* ctx);
static duk_ret_t js_GetPlayerKey            (duk_context* ctx);
static duk_ret_t js_GetToggleState          (duk_context* ctx);
static duk_ret_t js_SetMousePosition        (duk_context* ctx);
static duk_ret_t js_BindKey                 (duk_context* ctx);
static duk_ret_t js_BindJoystickButton      (duk_context* ctx);
static duk_ret_t js_ClearKeyQueue           (duk_context* ctx);
static duk_ret_t js_UnbindKey               (duk_context* ctx);
static duk_ret_t js_UnbindJoystickButton    (duk_context* ctx);

static void queue_key         (int keycode);
static void queue_wheel_event (int event);

static int                    s_is_button_bound[MAX_JOYSTICKS][MAX_JOY_BUTTONS];
static int                    s_is_key_bound[ALLEGRO_KEY_MAX];
static int                    s_button_down_scripts[MAX_JOYSTICKS][MAX_JOY_BUTTONS];
static int                    s_button_up_scripts[MAX_JOYSTICKS][MAX_JOY_BUTTONS];
static ALLEGRO_EVENT_QUEUE*   s_events;
static int                    s_key_down_scripts[ALLEGRO_KEY_MAX];
static int                    s_key_up_scripts[ALLEGRO_KEY_MAX];
static ALLEGRO_JOYSTICK*      s_joy_handles[MAX_JOYSTICKS];
static ALLEGRO_JOYSTICK_STATE s_joy_state[MAX_JOYSTICKS];
static struct key_queue       s_key_queue;
static ALLEGRO_KEYBOARD_STATE s_keyboard_state;
static bool                   s_last_button_state[MAX_JOYSTICKS][MAX_JOY_BUTTONS];
static bool                   s_last_key_state[ALLEGRO_KEY_MAX];
static ALLEGRO_MOUSE_STATE    s_mouse_state;
static int                    s_last_wheel_pos;
static int                    s_num_wheel_events = 0;
static int                    s_wheel_queue[255];

void
initialize_input(void)
{
	int c_buttons = MAX_JOY_BUTTONS * MAX_JOYSTICKS;

	int i;

	al_install_keyboard();
	al_install_mouse();
	al_install_joystick();
	s_events = al_create_event_queue();
	al_register_event_source(s_events, al_get_keyboard_event_source());
	al_register_event_source(s_events, al_get_mouse_event_source());
	al_register_event_source(s_events, al_get_joystick_event_source());
	for (i = 0; i < MAX_JOYSTICKS; ++i) {
		s_joy_handles[i] = al_get_joystick(i);
	}

	memset(s_is_button_bound, 0, c_buttons * sizeof(bool));
	memset(s_is_key_bound, 0, ALLEGRO_KEY_MAX * sizeof(bool));
	memset(s_button_down_scripts, 0, c_buttons * sizeof(int));
	memset(s_button_up_scripts, 0, c_buttons * sizeof(int));
	memset(s_key_down_scripts, 0, ALLEGRO_KEY_MAX * sizeof(int));
	memset(s_key_up_scripts, 0, ALLEGRO_KEY_MAX * sizeof(int));
	memset(s_last_button_state, 0, c_buttons * sizeof(bool));
	memset(s_last_key_state, 0, ALLEGRO_KEY_MAX * sizeof(bool));
	
	al_get_keyboard_state(&s_keyboard_state);
	al_get_mouse_state(&s_mouse_state);
	s_last_wheel_pos = s_mouse_state.z;
}

void
shutdown_input(void)
{
	al_destroy_event_queue(s_events);
	al_uninstall_joystick();
	al_uninstall_mouse();
	al_uninstall_keyboard();
}

bool
is_any_key_down(void)
{
	int i_key;

	for (i_key = 0; i_key < ALLEGRO_KEY_MAX; ++i_key) {
		if (al_key_down(&s_keyboard_state, i_key))
			return true;
	}
	return false;
}

bool
is_joy_button_down(int joy_index, int button)
{
	return s_joy_state[joy_index].button[button] > 0;
}

float
get_joy_axis(int joy_index, int axis_index)
{
	ALLEGRO_JOYSTICK* joystick;
	int               n_stick_axes;
	int               n_sticks;

	int i;

	if (!(joystick = s_joy_handles[joy_index])) return 0.0;
	n_sticks = al_get_joystick_num_sticks(joystick);
	for (i = 0; i < n_sticks; ++i) {
		n_stick_axes = al_get_joystick_num_axes(joystick, i);
		if (axis_index < n_stick_axes)
			return s_joy_state[joy_index].stick[i].axis[axis_index];
		axis_index -= n_stick_axes;
	}
	return 0.0;
}

int
get_joy_axis_count(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;
	int               n_axes;
	int               n_sticks;

	int i;
	
	if (!(joystick = s_joy_handles[joy_index])) return 0;
	n_sticks = al_get_joystick_num_sticks(joystick);
	n_axes = 0;
	for (i = 0; i < n_sticks; ++i)
		n_axes += al_get_joystick_num_axes(joystick, i);
	return n_axes;
}

int
get_joy_button_count(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;

	if (!(joystick = s_joy_handles[joy_index])) return 0;
	return al_get_joystick_num_buttons(joystick);
}

void
clear_key_queue(void)
{
	s_key_queue.num_keys = 0;
}

void
update_input(void)
{
	ALLEGRO_EVENT     event;
	bool              is_down;
	ALLEGRO_JOYSTICK* joystick;

	int i, j;

	// process Allegro input events
	while (al_get_next_event(s_events, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_KEY_CHAR:
			switch (event.keyboard.keycode) {
			case ALLEGRO_KEY_ENTER:
				if (event.keyboard.modifiers & ALLEGRO_KEYMOD_ALT
					|| event.keyboard.modifiers & ALLEGRO_KEYMOD_ALTGR)
				{
					toggle_fullscreen();
				}
				else {
					queue_key(event.keyboard.keycode);
				}
				break;
			case ALLEGRO_KEY_F10:
				toggle_fullscreen();
				break;
			case ALLEGRO_KEY_F11:
				toggle_fps_display();
				break;
			case ALLEGRO_KEY_F12:
				take_screenshot();
				break;
			default:
				queue_key(event.keyboard.keycode);
				break;
			}
		}
	}
	
	al_get_keyboard_state(&s_keyboard_state);
	al_get_mouse_state(&s_mouse_state);
	for (i = 0; i < MAX_JOYSTICKS; i++) {
		if (joystick = s_joy_handles[i])
			al_get_joystick_state(joystick, &s_joy_state[i]);
		else
			memset(&s_joy_state[i], 0, sizeof(ALLEGRO_JOYSTICK_STATE));
	}
	if (s_mouse_state.z > s_last_wheel_pos) queue_wheel_event(MOUSE_WHEEL_UP);
	if (s_mouse_state.z < s_last_wheel_pos) queue_wheel_event(MOUSE_WHEEL_DOWN);
	s_last_wheel_pos = s_mouse_state.z;
	for (i = 0; i < ALLEGRO_KEY_MAX; ++i) {
		if (!s_is_key_bound[i])
			continue;
		is_down = al_key_down(&s_keyboard_state, i);
		if (is_down && !s_last_key_state[i]) run_script(s_key_down_scripts[i], false);
		if (!is_down && s_last_key_state[i]) run_script(s_key_up_scripts[i], false);
		s_last_key_state[i] = is_down;
	}
	for (i = 0; i < MAX_JOYSTICKS; ++i) for (j = 0; j < MAX_JOY_BUTTONS; ++j) {
		if (!s_is_button_bound[i][j])
			continue;
		is_down = is_joy_button_down(i, j);
		if (is_down && !s_last_button_state[i][j]) run_script(s_button_down_scripts[i][j], false);
		if (!is_down && s_last_button_state[i][j]) run_script(s_button_up_scripts[i][j], false);
		s_last_button_state[i][j] = is_down;
	}
}

void
init_input_api(void)
{
	initialize_input();
	
	register_api_const(g_duktape, "PLAYER_1", 0);
	register_api_const(g_duktape, "PLAYER_2", 1);
	register_api_const(g_duktape, "PLAYER_3", 2);
	register_api_const(g_duktape, "PLAYER_4", 3);
	register_api_const(g_duktape, "PLAYER_KEY_MENU", PLAYER_KEY_MENU);
	register_api_const(g_duktape, "PLAYER_KEY_UP", PLAYER_KEY_UP);
	register_api_const(g_duktape, "PLAYER_KEY_DOWN", PLAYER_KEY_DOWN);
	register_api_const(g_duktape, "PLAYER_KEY_LEFT", PLAYER_KEY_LEFT);
	register_api_const(g_duktape, "PLAYER_KEY_RIGHT", PLAYER_KEY_RIGHT);
	register_api_const(g_duktape, "PLAYER_KEY_A", PLAYER_KEY_A);
	register_api_const(g_duktape, "PLAYER_KEY_B", PLAYER_KEY_B);
	register_api_const(g_duktape, "PLAYER_KEY_X", PLAYER_KEY_X);
	register_api_const(g_duktape, "PLAYER_KEY_Y", PLAYER_KEY_Y);
	register_api_const(g_duktape, "KEY_SHIFT", ALLEGRO_KEY_LSHIFT);
	register_api_const(g_duktape, "KEY_CTRL", ALLEGRO_KEY_LCTRL);
	register_api_const(g_duktape, "KEY_ALT", ALLEGRO_KEY_ALT);
	register_api_const(g_duktape, "KEY_UP", ALLEGRO_KEY_UP);
	register_api_const(g_duktape, "KEY_DOWN", ALLEGRO_KEY_DOWN);
	register_api_const(g_duktape, "KEY_LEFT", ALLEGRO_KEY_LEFT);
	register_api_const(g_duktape, "KEY_RIGHT", ALLEGRO_KEY_RIGHT);
	register_api_const(g_duktape, "KEY_APOSTROPHE", ALLEGRO_KEY_QUOTE);
	register_api_const(g_duktape, "KEY_BACKSLASH", ALLEGRO_KEY_BACKSLASH);
	register_api_const(g_duktape, "KEY_BACKSPACE", ALLEGRO_KEY_BACKSPACE);
	register_api_const(g_duktape, "KEY_CLOSEBRACE", ALLEGRO_KEY_CLOSEBRACE);
	register_api_const(g_duktape, "KEY_CAPSLOCK", ALLEGRO_KEY_CAPSLOCK);
	register_api_const(g_duktape, "KEY_COMMA", ALLEGRO_KEY_COMMA);
	register_api_const(g_duktape, "KEY_DELETE", ALLEGRO_KEY_DELETE);
	register_api_const(g_duktape, "KEY_END", ALLEGRO_KEY_END);
	register_api_const(g_duktape, "KEY_ENTER", ALLEGRO_KEY_ENTER);
	register_api_const(g_duktape, "KEY_EQUALS", ALLEGRO_KEY_EQUALS);
	register_api_const(g_duktape, "KEY_ESCAPE", ALLEGRO_KEY_ESCAPE);
	register_api_const(g_duktape, "KEY_HOME", ALLEGRO_KEY_HOME);
	register_api_const(g_duktape, "KEY_INSERT", ALLEGRO_KEY_INSERT);
	register_api_const(g_duktape, "KEY_MINUS", ALLEGRO_KEY_MINUS);
	register_api_const(g_duktape, "KEY_NUMLOCK", ALLEGRO_KEY_NUMLOCK);
	register_api_const(g_duktape, "KEY_OPENBRACE", ALLEGRO_KEY_OPENBRACE);
	register_api_const(g_duktape, "KEY_PAGEDOWN", ALLEGRO_KEY_PGDN);
	register_api_const(g_duktape, "KEY_PAGEUP", ALLEGRO_KEY_PGUP);
	register_api_const(g_duktape, "KEY_PERIOD", ALLEGRO_KEY_FULLSTOP);
	register_api_const(g_duktape, "KEY_SCROLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	register_api_const(g_duktape, "KEY_SEMICOLON", ALLEGRO_KEY_SEMICOLON);
	register_api_const(g_duktape, "KEY_SPACE", ALLEGRO_KEY_SPACE);
	register_api_const(g_duktape, "KEY_SLASH", ALLEGRO_KEY_SLASH);
	register_api_const(g_duktape, "KEY_TAB", ALLEGRO_KEY_TAB);
	register_api_const(g_duktape, "KEY_TILDE", ALLEGRO_KEY_TILDE);
	register_api_const(g_duktape, "KEY_F1", ALLEGRO_KEY_F1);
	register_api_const(g_duktape, "KEY_F2", ALLEGRO_KEY_F2);
	register_api_const(g_duktape, "KEY_F3", ALLEGRO_KEY_F3);
	register_api_const(g_duktape, "KEY_F4", ALLEGRO_KEY_F4);
	register_api_const(g_duktape, "KEY_F5", ALLEGRO_KEY_F5);
	register_api_const(g_duktape, "KEY_F6", ALLEGRO_KEY_F6);
	register_api_const(g_duktape, "KEY_F7", ALLEGRO_KEY_F7);
	register_api_const(g_duktape, "KEY_F8", ALLEGRO_KEY_F8);
	register_api_const(g_duktape, "KEY_F9", ALLEGRO_KEY_F9);
	register_api_const(g_duktape, "KEY_F10", ALLEGRO_KEY_F10);
	register_api_const(g_duktape, "KEY_F11", ALLEGRO_KEY_F11);
	register_api_const(g_duktape, "KEY_F12", ALLEGRO_KEY_F12);
	register_api_const(g_duktape, "KEY_A", ALLEGRO_KEY_A);
	register_api_const(g_duktape, "KEY_B", ALLEGRO_KEY_B);
	register_api_const(g_duktape, "KEY_C", ALLEGRO_KEY_C);
	register_api_const(g_duktape, "KEY_D", ALLEGRO_KEY_D);
	register_api_const(g_duktape, "KEY_E", ALLEGRO_KEY_E);
	register_api_const(g_duktape, "KEY_F", ALLEGRO_KEY_F);
	register_api_const(g_duktape, "KEY_G", ALLEGRO_KEY_G);
	register_api_const(g_duktape, "KEY_H", ALLEGRO_KEY_H);
	register_api_const(g_duktape, "KEY_I", ALLEGRO_KEY_I);
	register_api_const(g_duktape, "KEY_J", ALLEGRO_KEY_J);
	register_api_const(g_duktape, "KEY_K", ALLEGRO_KEY_K);
	register_api_const(g_duktape, "KEY_L", ALLEGRO_KEY_L);
	register_api_const(g_duktape, "KEY_M", ALLEGRO_KEY_M);
	register_api_const(g_duktape, "KEY_N", ALLEGRO_KEY_N);
	register_api_const(g_duktape, "KEY_O", ALLEGRO_KEY_O);
	register_api_const(g_duktape, "KEY_P", ALLEGRO_KEY_P);
	register_api_const(g_duktape, "KEY_Q", ALLEGRO_KEY_Q);
	register_api_const(g_duktape, "KEY_R", ALLEGRO_KEY_R);
	register_api_const(g_duktape, "KEY_S", ALLEGRO_KEY_S);
	register_api_const(g_duktape, "KEY_T", ALLEGRO_KEY_T);
	register_api_const(g_duktape, "KEY_U", ALLEGRO_KEY_U);
	register_api_const(g_duktape, "KEY_V", ALLEGRO_KEY_V);
	register_api_const(g_duktape, "KEY_W", ALLEGRO_KEY_W);
	register_api_const(g_duktape, "KEY_X", ALLEGRO_KEY_X);
	register_api_const(g_duktape, "KEY_Y", ALLEGRO_KEY_Y);
	register_api_const(g_duktape, "KEY_Z", ALLEGRO_KEY_Z);
	register_api_const(g_duktape, "KEY_1", ALLEGRO_KEY_1);
	register_api_const(g_duktape, "KEY_2", ALLEGRO_KEY_2);
	register_api_const(g_duktape, "KEY_3", ALLEGRO_KEY_3);
	register_api_const(g_duktape, "KEY_4", ALLEGRO_KEY_4);
	register_api_const(g_duktape, "KEY_5", ALLEGRO_KEY_5);
	register_api_const(g_duktape, "KEY_6", ALLEGRO_KEY_6);
	register_api_const(g_duktape, "KEY_7", ALLEGRO_KEY_7);
	register_api_const(g_duktape, "KEY_8", ALLEGRO_KEY_8);
	register_api_const(g_duktape, "KEY_9", ALLEGRO_KEY_9);
	register_api_const(g_duktape, "KEY_0", ALLEGRO_KEY_0);
	register_api_const(g_duktape, "KEY_NUM_1", ALLEGRO_KEY_PAD_1);
	register_api_const(g_duktape, "KEY_NUM_2", ALLEGRO_KEY_PAD_2);
	register_api_const(g_duktape, "KEY_NUM_3", ALLEGRO_KEY_PAD_3);
	register_api_const(g_duktape, "KEY_NUM_4", ALLEGRO_KEY_PAD_4);
	register_api_const(g_duktape, "KEY_NUM_5", ALLEGRO_KEY_PAD_5);
	register_api_const(g_duktape, "KEY_NUM_6", ALLEGRO_KEY_PAD_6);
	register_api_const(g_duktape, "KEY_NUM_7", ALLEGRO_KEY_PAD_7);
	register_api_const(g_duktape, "KEY_NUM_8", ALLEGRO_KEY_PAD_8);
	register_api_const(g_duktape, "KEY_NUM_9", ALLEGRO_KEY_PAD_9);
	register_api_const(g_duktape, "KEY_NUM_0", ALLEGRO_KEY_PAD_0);

	register_api_const(g_duktape, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
	register_api_const(g_duktape, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
	register_api_const(g_duktape, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);
	register_api_const(g_duktape, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
	register_api_const(g_duktape, "MOUSE_WHEEL_UP", MOUSE_WHEEL_UP);
	register_api_const(g_duktape, "MOUSE_WHEEL_DOWN", MOUSE_WHEEL_DOWN);

	register_api_const(g_duktape, "JOYSTICK_AXIS_X", 0);
	register_api_const(g_duktape, "JOYSTICK_AXIS_Y", 1);
	register_api_const(g_duktape, "JOYSTICK_AXIS_Z", 2);
	register_api_const(g_duktape, "JOYSTICK_AXIS_R", 3);
	register_api_const(g_duktape, "JOYSTICK_AXIS_U", 4);
	register_api_const(g_duktape, "JOYSTICK_AXIS_V", 5);

	register_api_func(g_duktape, NULL, "AreKeysLeft", js_AreKeysLeft);
	register_api_func(g_duktape, NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed);
	register_api_func(g_duktape, NULL, "IsJoystickButtonPressed", js_IsJoystickButtonPressed);
	register_api_func(g_duktape, NULL, "IsKeyPressed", js_IsKeyPressed);
	register_api_func(g_duktape, NULL, "IsMouseButtonPressed", js_IsMouseButtonPressed);
	register_api_func(g_duktape, NULL, "GetJoystickAxis", js_GetJoystickAxis);
	register_api_func(g_duktape, NULL, "GetKey", js_GetKey);
	register_api_func(g_duktape, NULL, "GetKeyString", js_GetKeyString);
	register_api_func(g_duktape, NULL, "GetMouseWheelEvent", js_GetMouseWheelEvent);
	register_api_func(g_duktape, NULL, "GetMouseX", js_GetMouseX);
	register_api_func(g_duktape, NULL, "GetMouseY", js_GetMouseY);
	register_api_func(g_duktape, NULL, "GetNumJoysticks", js_GetNumJoysticks);
	register_api_func(g_duktape, NULL, "GetNumJoystickAxes", js_GetNumJoystickAxes);
	register_api_func(g_duktape, NULL, "GetNumJoystickButtons", js_GetNumJoystickButtons);
	register_api_func(g_duktape, NULL, "GetNumMouseWheelEvents", js_GetNumMouseWheelEvents);
	register_api_func(g_duktape, NULL, "GetPlayerKey", js_GetPlayerKey);
	register_api_func(g_duktape, NULL, "GetToggleState", js_GetToggleState);
	register_api_func(g_duktape, NULL, "SetMousePosition", js_SetMousePosition);
	register_api_func(g_duktape, NULL, "BindJoystickButton", js_BindJoystickButton);
	register_api_func(g_duktape, NULL, "BindKey", js_BindKey);
	register_api_func(g_duktape, NULL, "ClearKeyQueue", js_ClearKeyQueue);
	register_api_func(g_duktape, NULL, "UnbindJoystickButton", js_UnbindJoystickButton);
	register_api_func(g_duktape, NULL, "UnbindKey", js_UnbindKey);
}

static void
queue_key(int keycode)
{
	int key_index;

	if (s_key_queue.num_keys < 255) {
		key_index = s_key_queue.num_keys;
		++s_key_queue.num_keys;
		s_key_queue.keys[key_index] = keycode;
	}
}

static void
queue_wheel_event(int event)
{
	if (s_num_wheel_events < 255) {
		s_wheel_queue[s_num_wheel_events] = event;
		++s_num_wheel_events;
	}
}

static duk_ret_t
js_AreKeysLeft(duk_context* ctx)
{
	duk_push_boolean(ctx, s_key_queue.num_keys > 0);
	return 1;
}

static duk_ret_t
js_IsAnyKeyPressed(duk_context* ctx)
{
	int i_key;
	
	for (i_key = 0; i_key < ALLEGRO_KEY_MAX; ++i_key) {
		if (al_key_down(&s_keyboard_state, i_key)) {
			duk_push_true(ctx);
			return 1;
		}
	}
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_IsJoystickButtonPressed(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);
	
	duk_push_boolean(ctx, is_joy_button_down(joy_index, button));
	return 1;
}

static duk_ret_t
js_IsKeyPressed(duk_context* ctx)
{
	int code = duk_require_int(ctx, 0);

	duk_push_boolean(ctx, s_keyboard_state.display == g_display
		&& al_key_down(&s_keyboard_state, code));
	return 1;
}

static duk_ret_t
js_IsMouseButtonPressed(duk_context* ctx)
{
	int button = duk_require_int(ctx, 0);

	duk_push_boolean(ctx, s_mouse_state.display == g_display
		&& al_mouse_button_down(&s_mouse_state, button));
	return 1;
}

static duk_ret_t
js_GetJoystickAxis(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int axis_index = duk_require_int(ctx, 1);

	duk_push_number(ctx, get_joy_axis(joy_index, axis_index));
	return 1;
}

static duk_ret_t
js_GetKey(duk_context* ctx)
{
	int keycode;

	while (s_key_queue.num_keys == 0) {
		do_events();
	}
	keycode = s_key_queue.keys[0];
	--s_key_queue.num_keys;
	memmove(s_key_queue.keys, &s_key_queue.keys[1], sizeof(int) * s_key_queue.num_keys);
	duk_push_int(ctx, keycode);
	return 1;
}

static duk_ret_t
js_GetKeyString(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int keycode = duk_require_int(ctx, 0);
	bool shift = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;

	switch (keycode) {
	case ALLEGRO_KEY_A: duk_push_string(ctx, shift ? "A" : "a"); break;
	case ALLEGRO_KEY_B: duk_push_string(ctx, shift ? "B" : "b"); break;
	case ALLEGRO_KEY_C: duk_push_string(ctx, shift ? "C" : "c"); break;
	case ALLEGRO_KEY_D: duk_push_string(ctx, shift ? "D" : "d"); break;
	case ALLEGRO_KEY_E: duk_push_string(ctx, shift ? "E" : "e"); break;
	case ALLEGRO_KEY_F: duk_push_string(ctx, shift ? "F" : "f"); break;
	case ALLEGRO_KEY_G: duk_push_string(ctx, shift ? "G" : "g"); break;
	case ALLEGRO_KEY_H: duk_push_string(ctx, shift ? "H" : "h"); break;
	case ALLEGRO_KEY_I: duk_push_string(ctx, shift ? "I" : "i"); break;
	case ALLEGRO_KEY_J: duk_push_string(ctx, shift ? "J" : "j"); break;
	case ALLEGRO_KEY_K: duk_push_string(ctx, shift ? "K" : "k"); break;
	case ALLEGRO_KEY_L: duk_push_string(ctx, shift ? "L" : "l"); break;
	case ALLEGRO_KEY_M: duk_push_string(ctx, shift ? "M" : "m"); break;
	case ALLEGRO_KEY_N: duk_push_string(ctx, shift ? "N" : "n"); break;
	case ALLEGRO_KEY_O: duk_push_string(ctx, shift ? "O" : "o"); break;
	case ALLEGRO_KEY_P: duk_push_string(ctx, shift ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: duk_push_string(ctx, shift ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: duk_push_string(ctx, shift ? "R" : "r"); break;
	case ALLEGRO_KEY_S: duk_push_string(ctx, shift ? "S" : "s"); break;
	case ALLEGRO_KEY_T: duk_push_string(ctx, shift ? "T" : "t"); break;
	case ALLEGRO_KEY_U: duk_push_string(ctx, shift ? "U" : "u"); break;
	case ALLEGRO_KEY_V: duk_push_string(ctx, shift ? "V" : "v"); break;
	case ALLEGRO_KEY_W: duk_push_string(ctx, shift ? "W" : "w"); break;
	case ALLEGRO_KEY_X: duk_push_string(ctx, shift ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: duk_push_string(ctx, shift ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: duk_push_string(ctx, shift ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: duk_push_string(ctx, shift ? "!" : "1"); break;
	case ALLEGRO_KEY_2: duk_push_string(ctx, shift ? "@" : "2"); break;
	case ALLEGRO_KEY_3: duk_push_string(ctx, shift ? "#" : "3"); break;
	case ALLEGRO_KEY_4: duk_push_string(ctx, shift ? "$" : "4"); break;
	case ALLEGRO_KEY_5: duk_push_string(ctx, shift ? "%" : "5"); break;
	case ALLEGRO_KEY_6: duk_push_string(ctx, shift ? "^" : "6"); break;
	case ALLEGRO_KEY_7: duk_push_string(ctx, shift ? "&" : "7"); break;
	case ALLEGRO_KEY_8: duk_push_string(ctx, shift ? "*" : "8"); break;
	case ALLEGRO_KEY_9: duk_push_string(ctx, shift ? "(" : "9"); break;
	case ALLEGRO_KEY_0: duk_push_string(ctx, shift ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: duk_push_string(ctx, shift ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: duk_push_string(ctx, shift ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: duk_push_string(ctx, shift ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: duk_push_string(ctx, shift ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: duk_push_string(ctx, shift ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: duk_push_string(ctx, shift ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: duk_push_string(ctx, shift ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: duk_push_string(ctx, shift ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: duk_push_string(ctx, shift ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: duk_push_string(ctx, shift ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: duk_push_string(ctx, " "); break;
	case ALLEGRO_KEY_TAB: duk_push_string(ctx, "\t"); break;
	case ALLEGRO_KEY_TILDE: duk_push_string(ctx, shift ? "~" : "`"); break;
	default:
		duk_push_string(ctx, "");
	}
	return 1;
}

static duk_ret_t
js_GetMouseWheelEvent(duk_context* ctx)
{
	int event;

	int i;
	
	while (s_num_wheel_events == 0) {
		do_events();
	}
	if (s_num_wheel_events > 0) {
		event = s_wheel_queue[0];
		--s_num_wheel_events;
		for (i = 0; i < s_num_wheel_events; ++i) s_wheel_queue[i] = s_wheel_queue[i + 1];
	}
	duk_push_int(ctx, event);
	return 1;
}

static duk_ret_t
js_GetMouseX(duk_context* ctx)
{
	duk_push_int(ctx, s_mouse_state.x / g_scale_x);
	return 1;
}

static duk_ret_t
js_GetMouseY(duk_context* ctx)
{
	duk_push_int(ctx, s_mouse_state.y / g_scale_y);
	return 1;
}

static duk_ret_t
js_GetNumJoysticks(duk_context* ctx)
{
	duk_push_int(ctx, al_get_num_joysticks());
	return 1;
}

static duk_ret_t
js_GetNumJoystickAxes(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	
	duk_push_int(ctx, get_joy_axis_count(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumJoystickButtons(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);

	duk_push_int(ctx, get_joy_button_count(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumMouseWheelEvents(duk_context* ctx)
{
	duk_push_int(ctx, s_num_wheel_events);
	return 1;
}

static duk_ret_t
js_GetPlayerKey(duk_context* ctx)
{
	int key_type;
	int player;

	player = duk_require_int(ctx, 0);
	key_type = duk_require_int(ctx, 1);
	switch (key_type) {
	case PLAYER_KEY_MENU: duk_push_int(ctx, ALLEGRO_KEY_ESCAPE); break;
	case PLAYER_KEY_UP: duk_push_int(ctx, ALLEGRO_KEY_UP); break;
	case PLAYER_KEY_DOWN: duk_push_int(ctx, ALLEGRO_KEY_DOWN); break;
	case PLAYER_KEY_LEFT: duk_push_int(ctx, ALLEGRO_KEY_LEFT); break;
	case PLAYER_KEY_RIGHT: duk_push_int(ctx, ALLEGRO_KEY_RIGHT); break;
	case PLAYER_KEY_A: duk_push_int(ctx, ALLEGRO_KEY_Z); break;
	case PLAYER_KEY_B: duk_push_int(ctx, ALLEGRO_KEY_X); break;
	case PLAYER_KEY_X: duk_push_int(ctx, ALLEGRO_KEY_C); break;
	case PLAYER_KEY_Y: duk_push_int(ctx, ALLEGRO_KEY_V); break;
	}
	return 1;
}

static duk_ret_t
js_GetToggleState(duk_context* ctx)
{
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_SetMousePosition(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	al_set_mouse_xy(g_display, x * g_scale_x, y * g_scale_y);
	return 0;
}

static js_BindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);
	lstring_t* down_script = !duk_is_null(ctx, 2) ? duk_require_lstring_t(ctx, 2) : lstring_from_cstr("");
	lstring_t* up_script = !duk_is_null(ctx, 3) ? duk_require_lstring_t(ctx, 3) : lstring_from_cstr("");

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): Joystick index out of range (%i)", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): Button index out of range (%i)", button);
	free_script(s_button_down_scripts[joy_index][button]);
	free_script(s_button_up_scripts[joy_index][button]);
	s_button_down_scripts[joy_index][button] = compile_script(down_script, "[button-down script]");
	s_button_up_scripts[joy_index][button] = compile_script(up_script, "[button-up script]");
	free_lstring(down_script);
	free_lstring(up_script);
	s_is_button_bound[joy_index][button] = true;
	return 0;
}

static js_BindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);
	lstring_t* key_down_script = !duk_is_null(ctx, 1)
		? duk_require_lstring_t(ctx, 1) : lstring_from_cstr("");
	lstring_t* key_up_script = !duk_is_null(ctx, 2)
		? duk_require_lstring_t(ctx, 2) : lstring_from_cstr("");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindKey(): Invalid key constant");
	free_script(s_key_down_scripts[keycode]);
	free_script(s_key_up_scripts[keycode]);
	s_key_down_scripts[keycode] = compile_script(key_down_script, "[key-down script]");
	s_key_up_scripts[keycode] = compile_script(key_up_script, "[key-down script]");
	free_lstring(key_down_script);
	free_lstring(key_up_script);
	s_is_key_bound[keycode] = true;
	return 0;
}

static duk_ret_t
js_ClearKeyQueue(duk_context* ctx)
{
	s_key_queue.num_keys = 0;
	return 0;
}

static duk_ret_t
js_UnbindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): Joystick index out of range (%i)", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): Button index out of range (%i)", button);
	free_script(s_button_down_scripts[joy_index][button]);
	free_script(s_button_up_scripts[joy_index][button]);
	s_button_down_scripts[joy_index][button] = 0;
	s_button_up_scripts[joy_index][button] = 0;
	s_is_button_bound[joy_index][button] = false;
	return 0;
}

static duk_ret_t
js_UnbindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "UnbindKey(): Invalid key constant");
	free_script(s_key_down_scripts[keycode]);
	free_script(s_key_up_scripts[keycode]);
	s_key_down_scripts[keycode] = 0;
	s_key_up_scripts[keycode] = 0;
	s_is_key_bound[keycode] = false;
	return 0;
}
