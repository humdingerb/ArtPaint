/* 

	Filename:	BlurTool.cpp
	Contents:	StraigLineTool-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <CheckBox.h>

#include "BlurTool.h"
#include "Selection.h"
#include "StringServer.h"
#include "Cursors.h"


BlurTool::BlurTool()
	: DrawingTool(StringServer::ReturnString(BLUR_TOOL_NAME_STRING),BLUR_TOOL)
{
	options = SIZE_OPTION | CONTINUITY_OPTION;
	number_of_options = 2;
	
	SetOption(SIZE_OPTION,1);
	SetOption(CONTINUITY_OPTION,B_CONTROL_OFF);
}


BlurTool::~BlurTool()
{

}


ToolScript* BlurTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint)
{
	/*
		This function uses a convolution matrix to do the blurring.
		The matrix is following:
		
				1/9		1/9		1/9
				
				1/9		1/9		1/9
				
				1/9		1/9		1/9	 
	*/
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);
	
	BPoint prev_point;
	BWindow *window = view->Window();
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(bitmap);

	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));
	
	selection = view->GetSelection();
			
	BRect bounds = bitmap->Bounds();
	uint32 *bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow()/4;
	
	// this is the bitmap where the blurred image will be first made 
	BBitmap *blurred = new BBitmap(BRect(0,0,settings.size+1,settings.size+1),B_RGB_32_BIT);		
	int32 *blurred_bits;
	int32 blurred_bpr = blurred->BytesPerRow()/4;
	int32 previous_size = settings.size;
	
	float half_size;
	BRect rc;
	// for the quick calculation of square-roots
	int32 sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = (int32)sqrt(i);
	
	half_size = settings.size/2;
	prev_point = point - BPoint(1,1);
	last_updated_rect = BRect(point,point);
	while (buttons) {
		if ((settings.continuity == B_CONTROL_ON) || (settings.size != previous_size) || (point != prev_point)) {
			if (settings.size != previous_size) {
				delete blurred;
				half_size = settings.size/2;
				blurred = new BBitmap(BRect(0,0,settings.size+1,settings.size+1),B_RGB_32_BIT);
				previous_size = settings.size;
			}
			
			blurred_bits = (int32*)blurred->Bits();
	
			rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);		
			rc = rc & bounds;
	
			BPoint left_top = rc.LeftTop();
			uint32 new_pixel;
			float red,green,blue,alpha;
			int32 x_dist,y_sqr;
			for (int32 y=0;y<rc.Height()+1;y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				for (int32 x=0;x<rc.Width()+1;x++) {
					x_dist = (int32)(point.x-rc.left-x);
					if ((sqrt_table[x_dist*x_dist + y_sqr] <= half_size) &&
						(selection->ContainsPoint(left_top+BPoint(x,y))) ) {
						red=0;green=0;blue=0;alpha=0;
						for (int32 dy=-1;dy<2;dy++) {
							for (int32 dx=-1;dx<2;dx++) {
								int32 x_coord = (int32)min_c(max_c(left_top.x+x+dx,0),bounds.right);
								int32 y_coord = (int32)min_c(max_c(left_top.y+y+dy,0),bounds.bottom); 
								new_pixel = drawer->GetPixel(x_coord,y_coord);
								
								blue += (float)((new_pixel>>24)&0xFF)/9.0;
								green += (float)((new_pixel>>16)&0xFF)/9.0; 
								red += (float)((new_pixel>>8)&0xFF)/9.0;
								alpha += (float)((new_pixel)&0xFF)/9.0;
							}
						}
						// At this point we should round the values.
						*blurred_bits = (uint32)blue<<24 | (uint32)green<<16 | (uint32)red<<8 | (uint32)alpha;
					}
					else 
						*blurred_bits = drawer->GetPixel(left_top + BPoint(x,y));
						
					blurred_bits++;
				}
				blurred_bits += blurred_bpr - (int32)rc.Width();
			}
	
			blurred_bits = (int32*)blurred->Bits();
			if (rc.IsValid()) {
				for (int32 y=0;y<rc.Height()+1;y++) {
					for (int32 x=0;x<rc.Width()+1;x++) {
						*(bits_origin + (int32)(left_top.x + x) + (int32)((left_top.y+y)*bpr)) = *blurred_bits;
						blurred_bits++; 
					}
					blurred_bits += blurred_bpr - (int32)rc.Width();
				}
			}
	
		
			prev_point = point;
			the_script->AddPoint(point);
		}
		else
			rc = BRect(0,0,-1,-1);

		window->Lock();
		if (rc.IsValid()) {
			view->UpdateImage(rc);
			view->Sync();
			last_updated_rect = last_updated_rect | rc;
		}
		view->getCoords(&point,&buttons);
		window->Unlock();		
			
		snooze(20 * 1000);
	}
	delete blurred;
	delete drawer;		
	return the_script;
}


int32 BlurTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}

BView* BlurTool::makeConfigView()
{
	BlurToolConfigView *target_view = new BlurToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


const char* BlurTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(BLUR_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(BLUR_TOOL_IN_USE_STRING);
}

const void* BlurTool::ReturnToolCursor()
{
	return HS_BLUR_CURSOR;
}

BlurToolConfigView::BlurToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	// The ownership of this message is then transferred to the controller.
	BMessage *message;

	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);

	// First add the controller for size.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));	
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,100);			
	AddChild(size_slider);

	// Then add checkboxes for continuity and transparency blurring.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",CONTINUITY_OPTION);
	message->AddInt32("value",0x00000000);
	controller_frame = size_slider->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	continuity_checkbox = new BCheckBox(controller_frame,"continuity",StringServer::ReturnString(CONTINUOUS_STRING),message);	
	AddChild(continuity_checkbox);
	continuity_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(CONTINUITY_OPTION) != B_CONTROL_OFF) {
		continuity_checkbox->SetValue(B_CONTROL_ON);
	}	
	
	
	
	ResizeTo(max_c(continuity_checkbox->Frame().right,size_slider->Frame().right)+EXTRA_EDGE,continuity_checkbox->Frame().bottom + EXTRA_EDGE);
}


void BlurToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
	continuity_checkbox->SetTarget(BMessenger(this));
	
}