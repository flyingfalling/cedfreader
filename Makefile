
EDFAPIH=/usr/include/EyeLink/

cedfreader.exe: cedfreader.cpp
	g++ -std=c++20 -I$(EDFAPIH) -o cedfreader.exe cedfreader.cpp -ledfapi
