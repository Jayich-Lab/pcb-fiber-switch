const int BAUDRATE = 57600;
const int TIMEOUT = 50;
const int MIN_SWITCH_TIME = 1000;

const int CHAN_NUM = 8;

const int ERROR_COMMAND_SHORT = 0;
const int ERROR_CHAN_NOT_VALID = 1;
const int ERROR_COMMAND_NOT_SET_OR_GET = 2;
const int ERROR_COMMAND_CHAR_NOT_FOUND = 3;
const int ERROR_STATE_NOT_FOUND = 4;
const int ERROR_TIME_NOT_VALID = 5;

unsigned long time_now = 0;
unsigned long times[CHAN_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};

int chans[CHAN_NUM] = {9, 8, 7, 6, 5, 4, 3, 2};
unsigned int switch_times[CHAN_NUM] = {1000, 2000, 3000, 4000,
                                        5000, 6000, 7000, 8000};
bool auto_switches[CHAN_NUM] = {false, false, false, false,
                                false, false, false, false};
bool manual_switch_overrides[CHAN_NUM] = {true, true, true, true,
                                          true, true, true, true};
bool manual_switches[CHAN_NUM] = {false, false, false, false,
                                  false, false, false, false};
bool manual_switches_updated[CHAN_NUM] = {false, false, false, false,
                                          false, false, false, false};

int chan_to_int(char chan)
{
	return chan - '1';
}

String error_msg(int error_code)
{
	return String("ERROR ") + String(error_code);
}

String execute_set_state(int chan, String state)
{
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
		return error_msg(ERROR_STATE_NOT_FOUND);
	}
	return state;
}

String execute_set_switch_time(int chan, String switch_time_str)
{
	unsigned int switch_time = switch_time_str.toInt();
	if (switch_time < MIN_SWITCH_TIME)
	{
		return error_msg(ERROR_TIME_NOT_VALID);
	}
	else
	{
		switch_times[chan] = switch_time;
		times[chan] = 0;
		return switch_time_str;
	}
}

String execute_set(char command_char, int chan, String optional_input)
{
	if (command_char == 'S')
	{
		return execute_set_state(chan, optional_input);
	}
	else if (command_char == 'T')
	{
		return execute_set_switch_time(chan, optional_input);
	}
	else
	{
		return error_msg(ERROR_COMMAND_CHAR_NOT_FOUND);
	}
}

String execute_get(char command_char, int chan)
{
	if (command_char == 'S')
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
	else if (command_char == 'T')
	{
		return String(switch_times[chan]);
	}
	else
	{
		return error_msg(ERROR_COMMAND_CHAR_NOT_FOUND);
	}
}

bool parse_command(String command, char* command_char, int* chan, char* operation_char, String* optional_input)
{
	if (command.length() > 2)
	{
		*command_char = command.charAt(0);
		*chan = chan_to_int(command.charAt(1));
		*operation_char = command.charAt(2);
		if (*operation_char == ',')
		{
			*optional_input = command.substring(3);
		}
		return true;
	}
	else
	{
		return false;
	}
}

String execute_command(String command)
{
	char command_char = 0;
	int chan = -1;
	char operation_char = 0;
	String optional_input = String("");

	if (parse_command(command, &command_char, &chan, &operation_char, &optional_input))
	{
		if (chan >= 0 && chan < CHAN_NUM)
		{
			if (operation_char == '?')
			{
				return execute_get(command_char, chan);
			}
			else if (operation_char == ',')
			{
				return execute_set(command_char, chan, optional_input);
			}
			else
			{
				return error_msg(ERROR_COMMAND_NOT_SET_OR_GET);
			}
		}
		else
		{
			return error_msg(ERROR_CHAN_NOT_VALID);
		}
	}
	else
	{
		return error_msg(ERROR_COMMAND_SHORT);
	}
}

void setup()
{
	Serial.begin(BAUDRATE);
	Serial.setTimeout(TIMEOUT);
	time_now = millis();
	for (int i = 0; i < CHAN_NUM; i++)
	{
		pinMode(chans[i], OUTPUT);
    if (manual_switch_overrides[i])
    {
		digitalWrite(chans[i], manual_switches[i]);
    }
    else
    {
    digitalWrite(chans[i], auto_switches[i]);
    }
		times[i] = time_now;
	}
}

void loop()
{
	if (Serial.available())
	{
		String command = Serial.readStringUntil('\n');
		if (command.endsWith(String("\r")))
		{
			command = command.substring(0, command.length() - 1);
		}
		String output = execute_command(command);
		Serial.println(output);
	}
	
	unsigned long temp = millis();
	unsigned long time_step = temp - time_now;
	time_now = temp;
	for (int i = 0; i < CHAN_NUM; i++)
	{
		if (manual_switch_overrides[i])
		{
			if (manual_switches_updated[i])
			{
				manual_switches_updated[i] = false;
				digitalWrite(chans[i], manual_switches[i]);
			}
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
}
