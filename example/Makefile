all: vgm2wav
	./vgm2wav bgm_scc.vgm bgm_scc.wav

vgm2wav: vgm2wav.cpp ../sccvgm.hpp
	g++ -O2 -Wall -o vgm2wav vgm2wav.cpp
