#ifndef __XENMAPPER__
#define __XENMAPPER__

#include "IPlug_include_in_plug_hdr.h"
#include "IMidiQueue.h"
#include "scala-import.h"

class XenMapper : public IPlug
{
public:
  XenMapper(IPlugInstanceInfo instanceInfo);
  ~XenMapper();
  
  void ProcessMidiMsg(IMidiMsg* pMsg);
  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void ChannelMap_Add(IMidiMsg* pMsg);
  void ChannelMap_Remove(IMidiMsg* pMsg);
  void ChannelMap_Reset();
  void MIDIMapFreq_Generate();
  void TuningMap_Generate();
  bool SerializeState(ByteChunk * pChunk);
  int UnserializeState(ByteChunk* pChunk, int startPos);
  void PresetsChangedByHost();
 
private:
	
	enum EParams
	{
		kSCLFileSelector = 0,
		kKBDFileSelector,
		kICaptionControl,
		kChannelKnob,
		kNumParams
	};

	enum ELayout
	{
		kWidth = GUI_WIDTH,
		kHeight = GUI_HEIGHT,
		kChannelKnobX = 220,
		kChannelKnobY = 180,
		kChannelKnobFrames = 16
	};

	IMidiQueue mMidiQueue;
	ScalaScaleFile scl;
	IBitmapControl * mControlPointer;
	IBitmapControl * mControlPtr;
	ITextControl* scldesc;
	ITextControl* tuning_map_text;
	IGraphics* pGraphics;
	
	// Channel Map
	struct ChannelMap 
	{
		int polyphony = 1;
		int state[16];	// 0=free for assignment, 1=in use, 2=delayed stop
		int note[16];	// Destination note
		int bend[16];	// Pitch bend value to be applied for destination note
		int age[16];	// Time that the note was entered into the channel
	} chmap;
 
	// Tuning Map 
	struct TuningMap 
	{
		double freq[128];
		int note[128];
		double pitchbend[128];
	} tunmap;

	double midimap_freq[128];
	//int pitch_bend_cents[16];
	
	// Pitch bend range values:
	// 0 = .5 Semitone range (163.84 cents per bend) 
	// 1 =  1 Semitone range (81.92 cents per bend)
	// 2 =  2 Semitone range (40.96 cents per bend)
	// 3 =  4 Semitone range (20.48 cents per bend)
	int pitch_bend_range = 2; 
	double pitch_bend_cents[4] = { 163.84, 81.92, 40.96, 20.48 };


	int polyphony = 4;
	
	//General Flags	
	int dr_flag = 0; // Flag to keep IInvisibleSwitch from firing up the SCL file open dialog on plugin load - this is set to 1 in ProcessDoubleReplacing
	int scl_loaded = 0; //Flag to deternine whether to load inital values for the SCL fields
	char tuning_map_string[65536];
	int tuning_map_changed = 1;

	IInvisibleSwitchControl *scl_dialog;
	double WDL_FIXALIGN mFreq;
	double WDL_FIXALIGN mGain;
	double WDL_FIXALIGN mPhase; // Value in [0, 1].
	double WDL_FIXALIGN mSamplePeriod;


    
};

#endif
