#include "XenMapper.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "tinyfiledialogs.h"


const int kNumPrograms = 1;

XenMapper::XenMapper(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  //TRACE;

  if (!scl_loaded)
  {
	  scl_loaded = 1;
	  // Initialize 12EDO as base scale
	  sprintf(scl.description, "24EDO\n");
	  scl.notes = 24;
	  scl.cents[0] = 50.;
	  scl.cents[1] = 100.;
	  scl.cents[2] = 150.;
	  scl.cents[3] = 200.;
	  scl.cents[4] = 250.;
	  scl.cents[5] = 300.;
	  scl.cents[6] = 350.;
	  scl.cents[7] = 400.;
	  scl.cents[8] = 450.;
	  scl.cents[9] = 500.;
	  scl.cents[10] = 550.;
	  scl.cents[11] = 600.;
	  scl.cents[12] = 650.;
	  scl.cents[13] = 700.;
	  scl.cents[14] = 750.;
	  scl.cents[15] = 800.;
	  scl.cents[16] = 850.;
	  scl.cents[17] = 900.;
	  scl.cents[18] = 950.;
	  scl.cents[19] = 1000.;
	  scl.cents[20] = 1050.;
	  scl.cents[21] = 1100.;
	  scl.cents[22] = 1150.;
	  scl.cents[23] = 1200.;

	  scl.ratio[0] = 1.0594630943593;
	  scl.ratio[1] = 1.12246204830937;
	  scl.ratio[2] = 1.18920711500272;
	  scl.ratio[3] = 1.25992104989487;
	  scl.ratio[4] = 1.33483985417003;
	  scl.ratio[5] = 1.4142135623731;
	  scl.ratio[6] = 1.49830707687668;
	  scl.ratio[7] = 1.5874010519682;
	  scl.ratio[8] = 1.68179283050743;
	  scl.ratio[9] = 1.78179743628068;
	  scl.ratio[10] = 1.88774862536339;
	  scl.ratio[11] = 2.;
  }
  
  GetParam(kSCLFileSelector)->InitBool("IInvisibleSwitchControl", false, "");

  // Initialize innteral data structures
  ChannelMap_Reset();
  MIDIMapFreq_Generate();
  TuningMap_Generate();
  
  pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  IText textProps(16, &COLOR_YELLOW, "Arial", IText::kStyleNormal, IText::kAlignCenter);
  IText textProps2(16, &COLOR_YELLOW, "Arial", IText::kStyleNormal, IText::kAlignNear);
  
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kChannelKnob)->InitDouble("Polyphony", 1., 1., 16., 1., "");

  IBitmap knob = pGraphics->LoadIBitmap(CH_KNOB_ID, CH_KNOB_FN, kChannelKnobFrames);
  pGraphics->AttachControl(new IKnobMultiControl(this, kChannelKnobX, kChannelKnobY, kChannelKnob, &knob));

  scldesc = new ITextControl(this, IRECT(0, 55, 639, 85), &textProps, scl.description);
  pGraphics->AttachControl(scldesc);
  tuning_map_text = new ITextControl(this, IRECT(0, 90, 639, 479), &textProps, tuning_map_string);
  //pGraphics->AttachControl(tuning_map_text);

  //scl_dialog = new IInvisibleSwitchControl(this, IRECT(11, 47, 629, 77), kSCLFileSelector);
  scl_dialog = new IInvisibleSwitchControl(this, IRECT(10, 30, 40, 45), kSCLFileSelector);

  pGraphics->AttachControl(scl_dialog);
  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... ); 
  MakeDefaultPreset((char *) "-", kNumPrograms);
  
 
}

XenMapper::~XenMapper() {}

void XenMapper::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	//double* output = outputs[0];
	//double* in1 = inputs[0];
	//double* in2 = inputs[1];
	//double* out1 = outputs[0];
	//double* out2 = outputs[1];
    

	dr_flag = 1;

	scldesc->SetTextFromPlug(scl.description);
	tuning_map_text->SetTextFromPlug(tuning_map_string);
	
	for (int offset = 0; offset < nFrames; ++offset)
	{
		// Handle any MIDI messages in the queue.
		while (!mMidiQueue.Empty())
		{
			IMidiMsg* pMsg = mMidiQueue.Peek();
			// Stop when we've reached the current sample frame (offset).
			if (pMsg->mOffset > offset)
				break;

			// Handle the MIDI message.
			int status = pMsg->StatusMsg();
			int curr_note = pMsg->NoteNumber();
			int curr_vel = pMsg->Velocity();
			
			switch (status)
			{
				case IMidiMsg::kNoteOn:
					switch(curr_vel)
					{
						case 0:
							ChannelMap_Remove(pMsg);
							break;
						
						default:
							ChannelMap_Add(pMsg);
							break;
					}
					break;
				
				case IMidiMsg::kNoteOff:
					ChannelMap_Remove(pMsg);
					break;

				case IMidiMsg::kPitchWheel:
					break;
				
			}

			mMidiQueue.Remove();
		}
	}

	// Update the offsets of any MIDI messages still in the queue.
	mMidiQueue.Flush(nFrames);

}

void XenMapper::Reset()
{
  TRACE;
  IMutexLock lock(this);

  mSampleRate = GetSampleRate();
  mMidiQueue.Resize(GetBlockSize());

}

void XenMapper::OnParamChange(int paramIdx)
{
	IMutexLock lock(this);


  switch (paramIdx)
  {
 	  case kSCLFileSelector:

	  if ( GetGUI() && dr_flag)
	  {
		  // Zero out the Invisible Switch State
		  GetGUI()->SetParameterFromPlug(kSCLFileSelector, 0.0, 1);


		  int ret = 0;
		  char const * sclfilename;
		  char const * lFilterPatterns[2] = { "*.scl", "*." };
		  sclfilename = tinyfd_openFileDialog("Open SCL File", "", 2, lFilterPatterns, "SCALA Scale Files", 0);

		  if (sclfilename)
			  ret = scl.import_scl_file(sclfilename);

		  if (ret<0)
		  {
			  switch (ret)
			  {
			  case 0:
				  tinyfd_messageBox("Error Loading SCALA File", "Unable to read .scl file.", "ok", "error", 1);
				  break;
			  case -1:
				  tinyfd_messageBox("Error Loading SCALA File", "Unable to read description field.", "ok", "error", 1);
				  break;
			  case -2:
				  tinyfd_messageBox("Error Loading Scala File", "Unable to read number of notes in scale.", "ok", "error", 1);
				  break;
			  case -3:
				  tinyfd_messageBox("Error Loading SCALA File", "Unable to load scale ratios/cents of scale.", "ok", "error", 1);
				  break;
			  case -4:
				  tinyfd_messageBox("Error Loading SCALA File", "Unspecified Error", "ok", "error", 1);
				  break;
			  default:
				  tinyfd_messageBox("Error Loading SCALA File", "Unspecified Error", "ok", "error", 1);
				  break;
			  } //End if (ret<0)
		  
		  } //End if (ret<0)
	  
		  TuningMap_Generate();


	  }  // End if ( GetGUI() && dr_flag)
	 
	  break; // kSCLFileSelector

	  case kChannelKnob:
 		  ChannelMap_Reset();
		  chmap.polyphony = round(GetParam(kChannelKnob)->Value());
		  break;
	  
  }  // End switch (paramIdx)
}

void XenMapper::ProcessMidiMsg(IMidiMsg* pMsg) 
{
	switch (pMsg->StatusMsg())
	{
	
	case IMidiMsg::kNoteOn:
	case IMidiMsg::kNoteOff:
	case IMidiMsg::kPitchWheel:
		mMidiQueue.Add(pMsg);
		break;

	// Passthrough all other MIDI messages
	default:
		SendMidiMsg(pMsg);
		break;
	}

}

void XenMapper::ChannelMap_Add(IMidiMsg* pMsg)
{
	IMidiMsg NoteMsg, PitchBendMsg;
	int curr_note;
	int curr_vel;
	int mapped_note;

	curr_note = pMsg->NoteNumber();
	curr_vel = pMsg->Velocity();
	
	// Iterate through channels
	for (int i = 0; i < chmap.polyphony; i++)
	{
		PitchBendMsg.MakePitchWheelMsg(tunmap.pitchbend[curr_note], i);
		NoteMsg.MakeNoteOnMsg(tunmap.note[curr_note], curr_vel, 0, i);

		// If note is presently being played do nothing
		if ((chmap.note[i] == tunmap.note[curr_note]) &&
			(chmap.bend[i] == tunmap.pitchbend[curr_note]) &&
			chmap.state[i] == 1)
			return;

		// If note is not being played, but there is a channel that
		// was previously used to play it then use that channel
		if ((chmap.note[i] == tunmap.note[curr_note]) &&
			(chmap.bend[i] == tunmap.pitchbend[curr_note]) &&
			chmap.state[i] == 0)
		{
			chmap.state[i] = 1;

			SendMidiMsg(&PitchBendMsg);
			SendMidiMsg(&NoteMsg);
			return;
		}
	}
	
	// Prefer channels with no previuous assigments for new notes
	for (int i = 0; i < chmap.polyphony; i++)
	{
		PitchBendMsg.MakePitchWheelMsg(tunmap.pitchbend[curr_note], i);
		NoteMsg.MakeNoteOnMsg(tunmap.note[curr_note], curr_vel, 0, i);

		if ( (chmap.state[i] == 0) && (chmap.note[i] == 128) )
		{
			chmap.state[i] = 1;
			chmap.note[i] = tunmap.note[curr_note];
			chmap.bend[i] = tunmap.pitchbend[curr_note];

			SendMidiMsg(&PitchBendMsg);
			SendMidiMsg(&NoteMsg);
			return;
		}
	}
		
	// As a last resort find the oldest channel and use that
	for (int i = 0; i < chmap.polyphony; i++)
	{
		PitchBendMsg.MakePitchWheelMsg(tunmap.pitchbend[curr_note], i);
		NoteMsg.MakeNoteOnMsg(tunmap.note[curr_note], curr_vel, 0, i);

		// Check if the channel is free to use for the incoming note
		if (chmap.state[i] == 0)
		{
			chmap.state[i] = 1;
			chmap.note[i] = tunmap.note[curr_note];
			chmap.bend[i] = tunmap.pitchbend[curr_note];

			SendMidiMsg(&PitchBendMsg);
			SendMidiMsg(&NoteMsg);
			return;
		}

	}  


}

void XenMapper::ChannelMap_Remove(IMidiMsg* pMsg)
{
	IMidiMsg NoteOffMsg;
	int curr_note;
	int mapped_note;
	int curr_vel;

	curr_note = pMsg->NoteNumber();
	curr_vel = pMsg->Velocity();

	for (int i = 0; i < chmap.polyphony; i++)
	{
		NoteOffMsg.MakeNoteOffMsg(chmap.note[i], 0, i);
		
		// If note is presently being played do nothing
		if ( (chmap.note[i] == tunmap.note[curr_note]) &&
			  chmap.state[i] == 1)
		{
			chmap.state[i] = 0;
			SendMidiMsg(&NoteOffMsg);
			break;
		}
			
	}

}

void XenMapper::ChannelMap_Reset()
{
	IMidiMsg NoteOffMsg;
	
	for (int i = 0; i < 16; i++)
	{
		if (chmap.state[i] == 1)
		{
			NoteOffMsg.MakeNoteOffMsg(chmap.note[i], 0, i);
			SendMidiMsg(&NoteOffMsg);
		}
		
		chmap.state[i] = 0; // 0=free for assignment, 1=in use, 2=delayed stop
		chmap.note[i] = 128; // 0...127 == MIDI notes, 128 == unnassigned 
		chmap.bend[i] = 8192; // Middle of pitch bend range
		chmap.age[i] = i;
	}
}

void XenMapper::MIDIMapFreq_Generate()
{
	midimap_freq[60] = 261.625565300599;
	int incr = 59;
	while (incr > -1)
	{
		midimap_freq[incr] = midimap_freq[incr + 1] / pow(2, (double)1 / (double)12);
		incr -= 1;
	}

	incr = 61;
	while (incr < 128)
	{
	midimap_freq[incr] = midimap_freq[incr - 1] * pow(2, (double)1 / (double)12);
	incr += 1;
	}
}

void XenMapper::TuningMap_Generate()
{
	// Generate Tuning Frequency Map
	double base_freq;
	int note_counter;
	int pitch_bend_temp;
	
	base_freq = 261.625565300599;
	tunmap.freq[60] = base_freq;

	note_counter = 0;
	int incr = 59;
	while (incr > -1)
	{
		tunmap.freq[incr] = base_freq/(pow(2, scl.cents[note_counter] / 1200));

		note_counter += 1;
		if (note_counter == scl.notes)
		{
			note_counter = 0;
			base_freq = tunmap.freq[incr];
		}

		incr -= 1;
	}

	base_freq = 261.625565300599;
	note_counter = 0;
	incr = 61;
	while (incr < 128)
	{
		tunmap.freq[incr] = base_freq*(pow(2,scl.cents[note_counter]/1200));

		note_counter += 1;
		if (note_counter == scl.notes)
		{
			note_counter = 0;
			base_freq = tunmap.freq[incr];
		}

		incr += 1;
	}

	// Generate Tuning Note Map
	int midi_incr;
	double temp_freq_diff;
	double new_freq_diff;

	int tun_incr = 0;

	while (tun_incr < 128)
	{
		midi_incr = 0;
		temp_freq_diff = fabs(tunmap.freq[tun_incr] - midimap_freq[midi_incr]);
		tunmap.note[tun_incr] = midi_incr;

		while (midi_incr < 128)
		{
			new_freq_diff = fabs(tunmap.freq[tun_incr] - midimap_freq[midi_incr]);
			if (new_freq_diff < temp_freq_diff)
			{
				tunmap.note[tun_incr] = midi_incr;
				temp_freq_diff = fabs(tunmap.freq[tun_incr] - midimap_freq[midi_incr]);
			}

			midi_incr += 1;
		}

		tun_incr += 1;

	}
	
	// Generate base pitch bend Values
	for (int i = 0; i < 128; i++)
	{
		int pitch_bend_temp = 1200 * 3.322038403*log10(midimap_freq[tunmap.note[i]] / tunmap.freq[i]);
		tunmap.pitchbend[i] = 8192 - (pitch_bend_cents[pitch_bend_range] * pitch_bend_temp);
		
		// Modify the pitch bend value to be between -1 and 1
		tunmap.pitchbend[i] -= 8192;
		tunmap.pitchbend[i] /= 8192;
	}

	
}


bool XenMapper::SerializeState(ByteChunk* pChunk)
{
	TRACE;
	IMutexLock lock(this);
	
	double v;

	v = scl.notes;
	pChunk->Put(&v);
	
	for (int i = 0; i< 512; i++)
	{
		v = scl.description[i];
		pChunk->Put(&v);
	}


	// serialize the multi-slider state state before serializing the regular params
	for (int i = 0; i< 1024; i++)
	{
		v = scl.cents[i];
		pChunk->Put(&v);
	}

	for (int i = 0; i< 1024; i++)
	{
		v = scl.ratio[i];
		pChunk->Put(&v);
	}

	return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int XenMapper::UnserializeState(ByteChunk* pChunk, int startPos)
{
	TRACE;
	IMutexLock lock(this);
	double v = 0.0;

	startPos = pChunk->Get(&v, startPos);
	scl.notes = v;

	for (int i = 0; i< 512; i++)
	{
		v = 0.0;
		startPos = pChunk->Get(&v, startPos);
		scl.description[i] = v;
	}


	for (int i = 0; i< 1024; i++)
	{
		v = 0.0;
		startPos = pChunk->Get(&v, startPos);
		scl.cents[i] = v;
	}

	for (int i = 0; i< 1024; i++)
	{
		v = 0.0;
		startPos = pChunk->Get(&v, startPos);
		scl.ratio[i] = v;
	}	
	
	scl_loaded = 1;

	//startPos = pChunk->GetStr(scl.description, startPos);

	TuningMap_Generate();

	return IPlugBase::UnserializeParams(pChunk, startPos); // must remember to call UnserializeParams at the end
}

void XenMapper::PresetsChangedByHost()
{
	TRACE;
	IMutexLock lock(this);

	if (GetGUI())
		GetGUI()->SetAllControlsDirty();
}


