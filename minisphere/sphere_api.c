#include "minisphere.h"

static void push_image_obj(duk_context* ctx, ALLEGRO_BITMAP* bitmap);
static void push_sound_obj(duk_context* ctx, ALLEGRO_AUDIO_STREAM* stream);
static void reg_script_func(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);

// Engine functions
static duk_ret_t duk_GetVersion(duk_context* ctx);
static duk_ret_t duk_GetVersionString(duk_context* ctx);
static duk_ret_t duk_GarbageCollect(duk_context* ctx);
static duk_ret_t duk_Abort(duk_context* ctx);
static duk_ret_t duk_EvaluateScript(duk_context* ctx);
static duk_ret_t duk_EvaluateSystemScript(duk_context* ctx);
static duk_ret_t duk_Exit(duk_context* ctx);
static duk_ret_t duk_GetTime(duk_context* ctx);
static duk_ret_t duk_RequireScript(duk_context* ctx);
static duk_ret_t duk_RequireSystemScript(duk_context* ctx);

// Color management functions
static duk_ret_t duk_CreateColor(duk_context* ctx);
static duk_ret_t duk_BlendColors(duk_context* ctx);
static duk_ret_t duk_BlendColorsWeighted(duk_context* ctx);

// Image functions
static duk_ret_t duk_LoadImage(duk_context* ctx);
static duk_ret_t duk_GrabImage(duk_context* ctx);
static duk_ret_t duk_Image_finalize(duk_context* ctx);
static duk_ret_t duk_Image_blit(duk_context* ctx);

// Graphics/rendering functions
static duk_ret_t duk_GetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_GetScreenHeight(duk_context* ctx);
static duk_ret_t duk_GetScreenWidth(duk_context* ctx);
static duk_ret_t duk_SetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_ApplyColorMask(duk_context* ctx);
static duk_ret_t duk_FlipScreen(duk_context* ctx);
static duk_ret_t duk_GradientRectangle(duk_context* ctx);
static duk_ret_t duk_OutlinedRectangle(duk_context* ctx);
static duk_ret_t duk_Rectangle(duk_context* ctx);

// Sound functions
static duk_ret_t duk_LoadSound(duk_context* ctx);
static duk_ret_t duk_Sound_finalize(duk_context* ctx);
static duk_ret_t duk_Sound_isPlaying(duk_context* ctx);
static duk_ret_t duk_Sound_isSeekable(duk_context* ctx);
static duk_ret_t duk_Sound_getLength(duk_context* ctx);
static duk_ret_t duk_Sound_getPan(duk_context* ctx);
static duk_ret_t duk_Sound_getPitch(duk_context* ctx);
static duk_ret_t duk_Sound_getPosition(duk_context* ctx);
static duk_ret_t duk_Sound_getRepeat(duk_context* ctx);
static duk_ret_t duk_Sound_getVolume(duk_context* ctx);
static duk_ret_t duk_Sound_setPan(duk_context* ctx);
static duk_ret_t duk_Sound_setPitch(duk_context* ctx);
static duk_ret_t duk_Sound_setPosition(duk_context* ctx);
static duk_ret_t duk_Sound_setRepeat(duk_context* ctx);
static duk_ret_t duk_Sound_setVolume(duk_context* ctx);
static duk_ret_t duk_Sound_clone(duk_context* ctx);
static duk_ret_t duk_Sound_pause(duk_context* ctx);
static duk_ret_t duk_Sound_play(duk_context* ctx);
static duk_ret_t duk_Sound_reset(duk_context* ctx);
static duk_ret_t duk_Sound_stop(duk_context* ctx);

void
init_sphere_api(duk_context* ctx)
{
	reg_script_func(ctx, NULL, "GetVersion", &duk_GetVersion);
	reg_script_func(ctx, NULL, "GetVersionString", &duk_GetVersionString);
	reg_script_func(ctx, NULL, "GarbageCollect", &duk_GarbageCollect);
	reg_script_func(ctx, NULL, "Abort", &duk_Abort);
	reg_script_func(ctx, NULL, "EvaluateScript", &duk_EvaluateScript);
	reg_script_func(ctx, NULL, "EvaluateSystemScript", &duk_EvaluateSystemScript);
	reg_script_func(ctx, NULL, "Exit", &duk_Exit);
	reg_script_func(ctx, NULL, "GetTime", &duk_GetTime);
	reg_script_func(ctx, NULL, "RequireScript", &duk_RequireScript);
	reg_script_func(ctx, NULL, "RequireSystemScript", &duk_RequireSystemScript);
	reg_script_func(ctx, NULL, "CreateColor", &duk_CreateColor);
	reg_script_func(ctx, NULL, "BlendColors", &duk_BlendColors);
	reg_script_func(ctx, NULL, "BlendColorsWeighted", &duk_BlendColorsWeighted);
	reg_script_func(ctx, NULL, "LoadImage", &duk_LoadImage);
	reg_script_func(ctx, NULL, "GrabImage", &duk_GrabImage);
	reg_script_func(ctx, NULL, "GetClippingRectangle", &duk_GetClippingRectangle);
	reg_script_func(ctx, NULL, "GetScreenHeight", &duk_GetScreenHeight);
	reg_script_func(ctx, NULL, "GetScreenWidth", &duk_GetScreenWidth);
	reg_script_func(ctx, NULL, "SetClippingRectangle", &duk_SetClippingRectangle);
	reg_script_func(ctx, NULL, "ApplyColorMask", &duk_ApplyColorMask);
	reg_script_func(ctx, NULL, "FlipScreen", &duk_FlipScreen);
	reg_script_func(ctx, NULL, "GradientRectangle", &duk_GradientRectangle);
	reg_script_func(ctx, NULL, "OutlinedRectangle", &duk_OutlinedRectangle);
	reg_script_func(ctx, NULL, "Rectangle", &duk_Rectangle);
	reg_script_func(ctx, NULL, "LoadSound", &duk_LoadSound);
	duk_push_global_stash(ctx);
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);
}

void
push_image_obj(duk_context* ctx, ALLEGRO_BITMAP* bitmap)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, bitmap); duk_put_prop_string(ctx, -2, "\xFF" "bitmap_ptr");
	duk_push_c_function(ctx, &duk_Image_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &duk_Image_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_string(ctx, "width"); duk_push_int(ctx, al_get_bitmap_width(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0 |
		DUK_DEFPROP_HAVE_VALUE | 0 |
		DUK_DEFPROP_HAVE_WRITABLE | 0);
	duk_push_string(ctx, "height"); duk_push_int(ctx, al_get_bitmap_height(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0 |
		DUK_DEFPROP_HAVE_VALUE | 0 |
		DUK_DEFPROP_HAVE_WRITABLE | 0);
}

void
push_sound_obj(duk_context* ctx, ALLEGRO_AUDIO_STREAM* stream)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, stream); duk_put_prop_string(ctx, -2, "\xFF" "stream_ptr");
	duk_push_c_function(ctx, &duk_Sound_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &duk_Sound_isPlaying, DUK_VARARGS); duk_put_prop_string(ctx, -2, "isPlaying");
	duk_push_c_function(ctx, &duk_Sound_isSeekable, DUK_VARARGS); duk_put_prop_string(ctx, -2, "isSeekable");
	duk_push_c_function(ctx, &duk_Sound_getRepeat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getRepeat");
	duk_push_c_function(ctx, &duk_Sound_getVolume, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getVolume");
	duk_push_c_function(ctx, &duk_Sound_setRepeat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setRepeat");
	duk_push_c_function(ctx, &duk_Sound_setVolume, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setVolume");
	duk_push_c_function(ctx, &duk_Sound_pause, DUK_VARARGS); duk_put_prop_string(ctx, -2, "pause");
	duk_push_c_function(ctx, &duk_Sound_play, DUK_VARARGS); duk_put_prop_string(ctx, -2, "play");
	duk_push_c_function(ctx, &duk_Sound_reset, DUK_VARARGS); duk_put_prop_string(ctx, -2, "reset");
	duk_push_c_function(ctx, &duk_Sound_stop, DUK_VARARGS); duk_put_prop_string(ctx, -2, "stop");
}

void
reg_script_func(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_get_prop_string(ctx, -1, ctor_name);
		duk_get_prop_string(ctx, -1, "prototype");
	}
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, name);
	if (ctor_name != NULL) {
		duk_pop_2(ctx);
	}
	duk_pop(ctx);
}

duk_ret_t
duk_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, 1.5);
	return 1;
}

duk_ret_t
duk_GetVersionString(duk_context* ctx)
{
	duk_push_sprintf(ctx, "minisphere %s (API: sphere-%s)", ENGINE_VER, SPHERE_API_VER);
	return 1;
}

duk_ret_t
duk_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

duk_ret_t
duk_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* err_msg = n_args > 0 ? duk_to_string(ctx, 0) : "Abort() called by script";
	duk_error(ctx, DUK_ERR_ERROR, "%s ", err_msg);
	return 0;
}

duk_ret_t
duk_EvaluateScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts");
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

duk_ret_t
duk_EvaluateSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "system/scripts");
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

duk_ret_t
duk_Exit(duk_context* ctx)
{
	duk_error(ctx, DUK_ERR_ERROR, "!exit");
	return 0;
}

duk_ret_t
duk_GetTime(duk_context* ctx)
{
	clock_t c_ticks = clock();
	double ms = (double)c_ticks / CLOCKS_PER_SEC * 1000;
	duk_push_number(ctx, ms);
	return 1;
}

duk_ret_t
duk_RequireScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_eval_file_noresult(ctx, script_path);
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

duk_ret_t
duk_RequireSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "system/scripts");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_eval_file_noresult(ctx, script_path);
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

duk_ret_t
duk_CreateColor(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int red_val = duk_get_int(ctx, 0);
	int green_val = duk_get_int(ctx, 1);
	int blue_val = duk_get_int(ctx, 2);
	int alpha_val = n_args > 3 ? duk_get_int(ctx, 3) : 255;
	duk_push_object(ctx);
	duk_push_int(ctx, red_val); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, green_val); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, blue_val); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, alpha_val); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

duk_ret_t
duk_BlendColors(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_int_t r1, g1, b1, a1;
	duk_int_t r2, g2, b2, a2;
	duk_get_prop_string(ctx, 0, "red"); r1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "green"); g1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "blue"); b1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "alpha"); a1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "red"); r2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "green"); g2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "blue"); b2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "alpha"); a2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_push_object(ctx);
	duk_push_int(ctx, (r1 + r2) / 2); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, (g1 + g2) / 2); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, (b1 + b2) / 2); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, (a1 + a2) / 2); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

duk_ret_t
duk_BlendColorsWeighted(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_int_t r1, g1, b1, a1;
	duk_int_t r2, g2, b2, a2;
	duk_get_prop_string(ctx, 0, "red"); r1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "green"); g1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "blue"); b1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "alpha"); a1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "red"); r2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "green"); g2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "blue"); b2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "alpha"); a2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_double_t w1 = duk_get_number(ctx, 2);
	duk_double_t w2 = duk_get_number(ctx, 3);
	duk_double_t sigma = w1 + w2;
	duk_push_object(ctx);
	duk_push_int(ctx, (r1 * w1 + r2 * w2) / sigma); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, (g1 * w1 + g2 * w2) / sigma); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, (b1 * w1 + b2 * w2) / sigma); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, (a1 * w1 + a2 * w2) / sigma); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

duk_ret_t
duk_LoadImage(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "images");
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(path);
	free(path);
	if (bitmap != NULL) {
		push_image_obj(ctx, bitmap);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadImage(): Unable to load image '%s'", filename);
	}
}

duk_ret_t
duk_GrabImage(duk_context* ctx)
{
	int x_res = al_get_display_width(g_display);
	int y_res = al_get_display_height(g_display);
	ALLEGRO_BITMAP* bitmap = al_create_bitmap(x_res, y_res);
	if (bitmap != NULL) {
		push_image_obj(ctx, bitmap);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "GrabImage(): Unable to create new bitmap");
	}
}

duk_ret_t
duk_Image_finalize(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_bitmap(bitmap);
	return 0;
}

duk_ret_t
duk_Image_blit(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	ALLEGRO_BITMAP* bitmap;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_draw_bitmap(bitmap, x, y, 0x0);
	return 1;
}

duk_ret_t
duk_GetClippingRectangle(duk_context* ctx)
{
	int x, y, width, height;
	al_get_clipping_rectangle(&x, &y, &width, &height);
	duk_push_object(ctx);
	duk_push_int(ctx, x); duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, y); duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, width); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height); duk_put_prop_string(ctx, -2, "height");
	return 1;
}

duk_ret_t
duk_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, (duk_int_t)al_get_display_height(g_display));
	return 1;
}

duk_ret_t
duk_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, (duk_int_t)al_get_display_width(g_display));
	return 1;
}

duk_ret_t
duk_SetClippingRectangle(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	float width = (float)duk_get_number(ctx, 2);
	float height = (float)duk_get_number(ctx, 3);
	al_set_clipping_rectangle(x, y, width, height);
	return 0;
}

duk_ret_t
duk_ApplyColorMask(duk_context* ctx)
{
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, -1, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	float width = (float)al_get_display_width(g_display);
	float height = (float)al_get_display_height(g_display);
	al_draw_filled_rectangle(0, 0, width, height, al_map_rgba(r, g, b, a));
	return 0;
}

duk_ret_t
duk_FlipScreen(duk_context* ctx)
{
	ALLEGRO_EVENT event;
	ALLEGRO_TIMEOUT timeout;
	al_init_timeout(&timeout, 0.05);
	bool got_event = al_wait_for_event_until(g_events, &event, &timeout);
	if (got_event && event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
		duk_error(ctx, DUK_ERR_ERROR, "!exit");
	}
	al_flip_display();
	al_clear_to_color(al_map_rgb(0, 0, 0));
	return 0;
}

duk_ret_t
duk_GradientRectangle(duk_context* ctx)
{
	float x1 = (float)duk_get_number(ctx, 0);
	float y1 = (float)duk_get_number(ctx, 1);
	float x2 = x1 + (float)duk_get_number(ctx, 2);
	float y2 = y1 + (float)duk_get_number(ctx, 3);
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ul = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 5, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ur = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 6, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_lr = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 7, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ll = al_map_rgba(r, g, b, a);
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, color_ul },
		{ x2, y1, 0, 0, 0, color_ur },
		{ x1, y2, 0, 0, 0, color_ll },
		{ x2, y2, 0, 0, 0, color_lr }
	};
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

duk_ret_t
duk_OutlinedRectangle(duk_context* ctx)
{
	duk_idx_t n_args = duk_get_top(ctx);
	float x1 = (float)duk_get_number(ctx, 0) + 0.5;
	float y1 = (float)duk_get_number(ctx, 1) + 0.5;
	float x2 = x1 + (float)duk_get_number(ctx, 2) - 1;
	float y2 = y1 + (float)duk_get_number(ctx, 3) - 1;
	float thickness = n_args >= 6 ? (float)floor((double)duk_get_number(ctx, 5)) : 1;
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	al_draw_rectangle(x1, y1, x2, y2, al_map_rgba(r, g, b, a), thickness);
	return 0;
}

duk_ret_t
duk_Rectangle(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	float width = (float)duk_get_number(ctx, 2);
	float height = (float)duk_get_number(ctx, 3);
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	al_draw_filled_rectangle(x, y, x + width, y + height, al_map_rgba(r, g, b, a));
	return 0;
}

duk_ret_t
duk_LoadSound(duk_context* ctx)
{
	duk_int_t n_args = duk_get_top(ctx);
	const char* filename = duk_get_string(ctx, 0);
	char* sound_path = get_asset_path(filename, "sounds");
	duk_bool_t is_stream = n_args >= 2 ? duk_get_boolean(ctx, 1) : true;
	ALLEGRO_AUDIO_STREAM* stream = al_load_audio_stream(sound_path, 4, 2048);
	free(sound_path);
	if (stream != NULL) {
		al_set_audio_stream_playing(stream, false);
		al_attach_audio_stream_to_mixer(stream, al_get_default_mixer());
		al_set_audio_stream_gain(stream, 1.0);
		push_sound_obj(ctx, stream);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadSound(): Unable to load sound file '%s'", filename);
	}
}

duk_ret_t
duk_Sound_finalize(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_get_prop_string(ctx, 0, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	al_detach_audio_stream(stream);
	al_destroy_audio_stream(stream);
	return 0;
}

duk_ret_t
duk_Sound_isPlaying(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, al_get_audio_stream_playing(stream));
	return 1;
}

duk_ret_t
duk_Sound_isSeekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

duk_ret_t
duk_Sound_getRepeat(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, al_get_audio_stream_playmode(stream) == ALLEGRO_PLAYMODE_LOOP);
	return 1;
}

duk_ret_t
duk_Sound_getVolume(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_number(ctx, al_get_audio_stream_gain(stream));
	return 1;
}

duk_ret_t
duk_Sound_setRepeat(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_bool_t is_looped = duk_get_boolean(ctx, 0);
	ALLEGRO_PLAYMODE play_mode = is_looped ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
	al_set_audio_stream_playmode(stream, play_mode);
	return 0;
}

duk_ret_t
duk_Sound_setVolume(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	float new_vol = duk_get_number(ctx, 0);
	al_set_audio_stream_gain(stream, new_vol);
	return 0;
}

duk_ret_t
duk_Sound_pause(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	return 0;
}

duk_ret_t
duk_Sound_play(duk_context* ctx)
{
	duk_idx_t n_args = duk_get_top(ctx);
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (n_args >= 1) {
		ALLEGRO_PLAYMODE play_mode = duk_get_boolean(ctx, 0)
			? ALLEGRO_PLAYMODE_LOOP
			: ALLEGRO_PLAYMODE_ONCE;
		al_seek_audio_stream_secs(stream, 0.0);
		al_set_audio_stream_playmode(stream, play_mode);
	}
	al_set_audio_stream_playing(stream, true);
	return 0;
}

duk_ret_t
duk_Sound_reset(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_seek_audio_stream_secs(stream, 0.0);
	al_set_audio_stream_playing(stream, true);
	return 0;
}

duk_ret_t
duk_Sound_stop(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	al_seek_audio_stream_secs(stream, 0.0);
	return 0;
}
