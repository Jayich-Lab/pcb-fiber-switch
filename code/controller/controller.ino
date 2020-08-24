const int num_chan = 8;
unsigned long time_now = 0;
unsigned long time_step = 0;
int chans[num_chan] = {2, 3, 4, 5, 6, 7, 8, 9};
unsigned long times[num_chan] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long switch_times[num_chan] = {1000, 2000, 3000, 4000,
                                        5000, 6000, 7000, 8000};
bool auto_switches[num_chan] = {false, false, false, false,
                                false, false, false, false};
bool manual_switch_overrides[num_chan] = {false, false, false, false,
                                          false, false, false, false};
bool manual_switches[num_chan] = {false, false, false, false,
                                  false, false, false, false};
bool manual_switches_updated[num_chan] = {false, false, false, false,
                                          false, false, false, false};

int chan_to_int(char chan)
{
	return chan - '1';
}

String error_msg(int error_code)
{
	while(Serial.available()) {	Serial.read(); }
	return String("ERROR ") + String(error_code);
}

String execute_set(char command, int chan)
{
	delay(10);
	if (Serial.read() == ',')
	{
		if (!Serial.available())
		{
			return error_msg(0);
		}
		if (command == 'S')
		{
			String state = Serial.readStringUntil('\n');
			if (state == "AUTO")
			{
				manual_switch_overrides[chan] = false;
			}
			else if (state == "HIGH" || state == "LOW")
			{
				manual_switch_overrides[chan] = true;
				manual_switches_updated[chan] = true;
				manual_switches[chan] = (state == "HIGH");
			}
			else
			{
				return error_msg(10);
			}
			return state;
		}
		else if (command == 'T')
		{
			String time_str = Serial.readStringUntil('\n');
			unsigned long switch_time = time_str.toInt();
			if (switch_time < 10)
			{
				return error_msg(20);
			}
			else
			{
				switch_times[chan] = switch_time;
				times[chan] = 0;
				return time_str;
			}
		}
		else
		{
			return error_msg(1);
		}
	}
	else
	{
		return error_msg(0);
	}
}

String execute_get(char command, int chan)
{
	if (Serial.read() == '\n')
	{
		if (command == 'S')
		{
			if (!manual_switch_overrides[chan])
			{
				return String("AUTO");
			}
			else if (manual_switches[chan])
			{
				return String("HIGH");
			}
			else
			{
				return String("LOW");
			}
		}
		else if (command == 'T')
		{
			return String(switch_times[chan]);
		}
		else
		{
			return error_msg(1);
		}
	}
	else
	{
		return error_msg(0);
	}
}

String execute_command(char command, char chan, char is_set)
{
	int chan_int = chan_to_int(chan);
	if (chan_int < 0 || chan_int >= num_chan)
	{
		return error_msg(2);
	}

	if (is_set == 'S')
	{
		return execute_set(command, chan_int);
	}
	else if (is_set == 'G')
	{
		return execute_get(command, chan_int);
	}
	else
	{
		return error_msg(3);
	}
}

void setup()
{
	Serial.begin(9600);
	time_now = millis();
	for (int i = 0; i < num_chan; i++)
	{
		pinMode(chans[i], OUTPUT);
		digitalWrite(chans[i], LOW);
		times[i] = time_now;
	}
}

void loop()
{
	if (Serial.available() > 3)
	{
		char command = Serial.read();
		char chan = Serial.read();
		char is_set = Serial.read();
		String output = execute_command(command, chan, is_set);
		Serial.print(output + String("\n"));
	}
	time_step = millis() - time_now;
	time_now = millis();
	for (int i = 0; i < num_chan; i++)
	{
		if (manual_switch_overrides[i] && manual_switches_updated[i])
		{
			manual_switches_updated[i] = false;
			digitalWrite(chans[i], manual_switches[i]);
		}
		if (manual_switch_overrides[i])
		{
			continue;
		}
		times[i] += time_step;
		while (times[i] > switch_times[i] * 2)
		{
			times[i] -= switch_times[i] * 2;
		}
		if (times[i] >= switch_times[i] && !auto_switches[i])
		{
			auto_switches[i] = true;
			digitalWrite(chans[i], HIGH);
		}
		else if (times[i] < switch_times[i] && auto_switches[i])
		{
			auto_switches[i] = false;
			digitalWrite(chans[i], LOW);
		}
	}
	delay(1);
}