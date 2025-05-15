/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	HudList.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017

*/

#include "HudList.h"

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	PF_ADD_POPUPX("Selection Hude_Mode", 4, 1, "hud_version1|hud_version2|hud_version3|hud_version4", 0, LIST_DISK_ID);


	AEFX_CLR_STRUCT(def);


	
	out_data->num_params = HudList_NUM_PARAMS;

	return err;
}

static PF_Err
HudCanvasChannel16(
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo	*giP	= reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF	= 0;
					


	return err;
}
#include <cmath>

constexpr auto PI = 3.14159265358979323846;

template<typename T>
const T& clamp(const T& v, const T& lo, const T& hi) {
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

void HudVersionV1_SoftEdges(
	A_long xL,
	A_long yL,
	A_long centerX,
	A_long centerY,
	int numLines,
	double arcRadius,
	double arcThickness,
	PF_Pixel8* inP,
	PF_Pixel8* outP,
	PF_Pixel8 color)
{
	A_long dx = xL - centerX;
	A_long dy = yL - centerY;
	double dist = sqrt(dx * dx + dy * dy);
	double angle = atan2((double)dy, (double)dx);

	if (angle < 0) angle += 2 * PI;

	double innerRadius = 40.0;
	double outerRadius = 80.0;

	double feather = 4.0;
	double featherAngle = 0.06;

	double lineOpacity = 0.0;

	for (int i = 0; i < numLines; i++) {
		double theta = (2 * PI / numLines) * i;
		double delta = fabs(angle - theta);
		if (delta > PI) delta = 2 * PI - delta;

		if (delta < featherAngle && dist >= innerRadius - feather && dist <= outerRadius + feather) {
			double angleFactor = 1.0 - (delta / featherAngle);

			double radiusFactor = 1.0;
			if (dist < innerRadius)
				radiusFactor = (dist - (innerRadius - feather)) / feather;
			else if (dist > outerRadius)
				radiusFactor = 1.0 - ((dist - outerRadius) / feather);

			radiusFactor = clamp(radiusFactor, 0.0, 1.0);

			double opacity = angleFactor * radiusFactor;
			if (opacity > lineOpacity)
				lineOpacity = opacity;
		}
	}

	bool isInOuterArc = false;
	struct Arc { double start; double end; };
	Arc arcs[4] = {
		{ PI * 0.1, PI * 0.9 },
		{ PI * 1.1, PI * 1.9 },
		{ PI * 2.1, PI * 2.9 },
		{ PI * 2.1, PI * 4.9 }
	};

	for (int i = 0; i < 3; i++) {
		if (angle > arcs[i].start && angle < arcs[i].end &&
			dist >= arcRadius && dist <= arcRadius + arcThickness) {
			isInOuterArc = true;
			break;
		}
	}

	bool isInCenterCircle = dist <= 20;

	if (lineOpacity > 0.0) {
		outP->red = color.red;
		outP->green = color.green;
		outP->blue = color.blue;
		outP->alpha = (A_u_char)(lineOpacity * 255);
	}
	else if (isInCenterCircle || isInOuterArc) {
		outP->red = color.red;
		outP->green = color.green;
		outP->blue = color.blue;
		outP->alpha = 255;
	}
	else {
		*outP = *inP;
	}
}


static PF_Err
HudCanvasChannel8(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err err = PF_Err_NONE;

	GainInfo* giP = reinterpret_cast<GainInfo*>(refcon);
	A_long centerX = giP->canvasX;
	A_long centerY = giP->canvasY;

	PF_Pixel8 hudColor;
	hudColor.red = 102;
	hudColor.green = 217;
	hudColor.blue = 230;
	hudColor.alpha = 255;


	switch (giP->modeInt) {
	case 1:
		HudVersionV1_SoftEdges(xL, yL, centerX, centerY, 12, 140.0, 18.0, inP, outP, hudColor);
		break;
	case 2:
		HudVersionV1_SoftEdges(xL, yL, centerX, centerY, 8, 180.0, 25, inP, outP, hudColor);
		break;
	case 3:
		HudVersionV1_SoftEdges(xL, yL, centerX, centerY, 8, 240, 90, inP, outP, hudColor);
		break;
	case 4:
		HudVersionV1_SoftEdges(xL, yL, centerX, centerY, 8, 130, 14, inP, outP, hudColor);
		break;
	}


	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	GainInfo			giP;
	AEFX_CLR_STRUCT(giP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;

	giP.canvasX = output->width / 2;
	giP.canvasY = output->height / 2;
	giP.modeInt = params[HudList_GAIN]->u.pd.value;
	
	
	if (PF_WORLD_IS_DEEP(output)){
		ERR(suites.Iterate16Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[HudList_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												HudCanvasChannel16,				// pixel function pointer
												output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[HudList_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												HudCanvasChannel8,				// pixel function pointer
												output));	
	}

	return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"HudList", // Name
		"ADBE HudList", // Match Name
		"Sample Plug-ins", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

