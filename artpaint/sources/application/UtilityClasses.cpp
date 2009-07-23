/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <InterfaceDefs.h>
#include <new>
#include <Resources.h>
#include <Screen.h>
#include <StringView.h>
#include <string.h>

#include "StringServer.h"
#include "UtilityClasses.h"


HelpWindow::HelpWindow(BPoint location, const char *text)
		:	BWindow(BRect(location,location),"A help window",B_BORDERED_WINDOW_LOOK,B_FLOATING_APP_WINDOW_FEEL,B_AVOID_FOCUS)
{
	BStringView *message_view = new BStringView(BRect(BPoint(1,1),BPoint(1,1)),"help text here",text);

	float width = message_view->StringWidth(text) + 2;
	font_height height;

	AddChild(message_view);
	message_view->GetFontHeight(&height);
	message_view->SetDrawingMode(B_OP_OVER);
	message_view->SetViewColor(tint_color(ui_color(B_WINDOW_TAB_COLOR),B_LIGHTEN_2_TINT));
	message_view->SetLowColor(tint_color(ui_color(B_WINDOW_TAB_COLOR),B_LIGHTEN_2_TINT));

	message_view->ResizeTo(width,height.ascent + height.descent + height.leading + 1);
	ResizeTo(message_view->Bounds().Width()+1,message_view->Bounds().Height());

	MoveBy(0,-(Bounds().Height()+3));

	// here we have to relocate ourselves in case we are not on screen
	BScreen *the_screen = new BScreen();
	BRect screen_bounds = the_screen->Frame();
	delete the_screen;

	if (!screen_bounds.Contains(Frame().LeftTop())) {
		// here we move the window down from cursor
		MoveBy(0,Bounds().Height()+3 + 16);
	}
	if (!screen_bounds.Contains(Frame().RightBottom())) {
		// here we move the window down from cursor
		MoveBy(-Bounds().Width(),0);
	}
}


HelpWindow::HelpWindow(BPoint location, char **text_lines, int32 line_count)
		:	BWindow(BRect(location,location),"A help window",B_BORDERED_WINDOW,0)
{
	BPoint left_top = BPoint(1,1);
	float width=0;
	font_height height;

	// here measure the height and width of strings
	BFont a_font = BFont();
	a_font.GetHeight(&height);

	for (int32 a=0;a<line_count;a++) {
		width = max_c(width,a_font.StringWidth(text_lines[a]));
	}
	width += 2;
	for (int32 i=0;i<line_count;i++) {
		BStringView *message_view = new BStringView(BRect(left_top,left_top),"help text here",text_lines[i]);
		AddChild(message_view);
		message_view->SetViewColor(tint_color(ui_color(B_WINDOW_TAB_COLOR),B_LIGHTEN_2_TINT));
		message_view->SetLowColor(tint_color(ui_color(B_WINDOW_TAB_COLOR),B_LIGHTEN_2_TINT));
		message_view->SetDrawingMode(B_OP_OVER);
		message_view->ResizeTo(width,height.ascent + height.descent);
		left_top += BPoint(0,height.ascent + height.descent);
	}

	ResizeTo(width+1,line_count*(height.ascent + height.descent));
	MoveBy(0,-(Bounds().Height()+3));
	// here we have to relocate ourselves in case we are not on screen
	BScreen *the_screen = new BScreen();
	BRect screen_bounds = the_screen->Frame();
	delete the_screen;

	if (!screen_bounds.Contains(Frame().LeftTop())) {
		// here we move the window down from cursor
		MoveBy(0,Bounds().Height()+3 + 16);
	}
	if (!screen_bounds.Contains(Frame().RightBottom())) {
		// here we move the window down from cursor
		MoveBy(-Bounds().Width(),0);
	}
}


// #pragma mark -- BitmapView


BitmapView::BitmapView(BBitmap* bitmap, BRect frame)
	: BView(frame, "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
	, fBitmap(bitmap)
{
}


BitmapView::BitmapView(BBitmap* bitmap, BPoint leftTop)
	: BView(BRect(leftTop, leftTop), "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
	, fBitmap(bitmap)
{
	if (fBitmap)
		ResizeTo(fBitmap->Bounds().Width(), fBitmap->Bounds().Height());
}


BitmapView::~BitmapView()
{
	delete fBitmap;
}


void
BitmapView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
BitmapView::Draw(BRect updateRect)
{
	if (fBitmap)
		DrawBitmap(fBitmap, updateRect, updateRect);
}


BBitmap*
BitmapView::Bitmap() const
{
	return fBitmap;
}


void
BitmapView::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = bitmap;
}


// #pragma mark -- BitmapViewBox


BitmapViewBox::BitmapViewBox(BBitmap *bitmap,BRect frame, char *label)
	:	BBox(frame)
{
	SetBorder(B_PLAIN_BORDER);

	bmap_view = new BitmapView(bitmap,BPoint(4,4));
	AddChild(bmap_view);

	ResizeTo(frame.Width(),bmap_view->Frame().Height()+2*4);

	BStringView *string_view = new BStringView(BRect(bmap_view->Frame().right+4,Bounds().top,Bounds().right-1,Bounds().bottom),"bitmap view label",label);
	AddChild(string_view);
	font_height fHeight;
	string_view->GetFontHeight(&fHeight);
	string_view->ResizeTo(string_view->Bounds().Width(),fHeight.ascent+fHeight.descent+fHeight.leading);
	string_view->MoveBy(0,(Bounds().Height()- string_view->Bounds().Height())/2.0);

}


void BitmapViewBox::UpdateBitmap()
{
	bmap_view->Invalidate();
}


BBitmap* CopyBitmap(BBitmap *to_be_copied,bool deep)
{
	BBitmap *new_bitmap;
	if (deep == TRUE) {
		new_bitmap = new BBitmap(to_be_copied->Bounds(),to_be_copied->ColorSpace(),TRUE);
	}
	else {
		new_bitmap = new BBitmap(to_be_copied->Bounds(),to_be_copied->ColorSpace());
	}
	if (new_bitmap->IsValid() == FALSE)
		throw std::bad_alloc();

	// Copy the bitmap data.
	uint32 *s_bits = (uint32*)to_be_copied->Bits();
	uint32 *d_bits = (uint32*)new_bitmap->Bits();

	int32 bitslength = to_be_copied->BitsLength()/4;
	for (int32 i=0;i<bitslength;i++) {
		*d_bits++ = *s_bits++;
	}
	return new_bitmap;
}


BRect FitRectToScreen(BRect source)
{
	// This function returns the source moved and resized so that it fits onto
	// current screen.
	BRect result = source;
	BRect screenFrame = BScreen().Frame();

	if (!screenFrame.Contains(result.RightBottom()))
		result.OffsetTo(BPoint(15.0, 15.0));

	if (!screenFrame.Contains(result.RightBottom())) {
		result.bottom = min_c(screenFrame.bottom, result.bottom);
		result.right = min_c(screenFrame.right, result.right);
	}

	return result;
}


BRect
CenterRectOnScreen(BRect source)
{
	BRect screenFrame = BScreen().Frame();

	BPoint leftTop((screenFrame.Width() + source.Width()) / 2.0,
		(screenFrame.Height() + source.Height()) / 2.0);

	if (leftTop.x < 0.0) leftTop.x = 0.0;
	if (leftTop.y < 0.0) leftTop.y = 0.0;

	return source.OffsetToCopy(leftTop);
}


BRect make_rect_from_points(BPoint &point1,BPoint &point2)
{
	BRect rect;
	rect.left = min_c(point1.x,point2.x);
	rect.right = max_c(point1.x,point2.x);
	rect.top = min_c(point1.y,point2.y);
	rect.bottom = max_c(point1.y,point2.y);

	return rect;
}
