#ifndef MINISPHERE__GEOMETRY_H__INCLUDED
#define MINISPHERE__GEOMETRY_H__INCLUDED

typedef struct point3 point3_t;
struct point3
{
	int x;
	int y;
	int z;
};

typedef struct rect rect_t;
struct rect
{
	int x1;
	int y1;
	int x2;
	int y2;
};

extern rect_t new_rect           (int x1, int y1, int x2, int y2);
extern bool   do_lines_intersect (rect_t a, rect_t b);
extern bool   do_rects_intersect (rect_t a, rect_t b);
extern bool   is_point_in_rect   (int x, int y, rect_t bounds);
extern rect_t translate_rect     (rect_t rect, int x_offset, int y_offset);
extern rect_t zoom_rect          (rect_t rect, double scale_x, double scale_y);

extern bool fread_rect_16 (FILE* file, rect_t* out_rect);
extern bool fread_rect_32 (FILE* file, rect_t* out_rect);

#endif // MINISPHERE__GEOMETRY_H__INCLUDED
