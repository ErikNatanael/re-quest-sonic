(
SynthDef(\windy, { |outBus = 0, knob = 0.5, decay = 10, gate = 1, amp = 1|
		var osc = BrownNoise.ar(0.5), env, focus, outs, freqs;
		env = EnvGen.kr(Env(
			levels: [0, 1, 0],
			times: [1, decay],
			releaseNode: 1),
		gate: gate,
		doneAction: 2);
		knob = knob*LFNoise1.kr(0.6).range(0.95, 1.1);
		freqs = [Lag.kr(knob, 0.4) * [2000, 2020] + 100, Lag.kr(knob, 0.05)*[100, 102]+60, Lag.kr(knob, 1.0)*500+40, Lag.kr(knob, 5)*300+30];
		focus = [LFNoise1.kr(4).range(0.001, 0.001), LFNoise2.kr(1.1).range(0.0001, 0.01), LFNoise2.kr(0.2).range(0.001, 0.05)];
		outs = Resonz.ar(osc, freqs, focus);
		outs = Mix.ar(outs);

		outs = (outs*100).tanh * 0.1;
		outs = HPF.ar(outs, \cutoff.kr(500));
		outs = outs * env*amp;

		Out.ar(outBus, [outs, DelayN.ar(outs, 0.01, 0.01)]);
	}).add;
)

( var port;
SerialPort.closeAll;
port = SerialPort.devices[0];
("Connecting to " + port).postln;
p = SerialPort(
	port,
	baudrate: 115200,
	crtscts: true);
~synth = Synth(\windy);

~readLoop = fork {
	var byte, str, peakFollower = 0;
	loop{|i|
		if(p.read==10, { var amp, knob;
			str = "";
			while({byte = p.read; byte !=13 }, {
				str= str++byte.asAscii;
			});

			str.postln;
			amp = str.asInt.linlin(400, 1000, 0.0, 1.0);
			("amp: " + amp.asString).postln;
			peakFollower = (str.asInt*0.001) + (peakFollower*0.999);
			knob = peakFollower.linlin(200, 1000, 1.0, 0.0);
			("knob: " + knob.asString).postln;
			~synth.set(\amp, amp);
			~synth.set(\knob, knob);
		});
	};

};
)
SerialPort.listDevices
SerialPort.cleanupAll
SerialPort.closeAll