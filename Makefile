CXXFLAGS += -Wall -pedantic --std=c++14 -DGLM_FORCE_RADIANS -g3 $(shell pkg-config --cflags freetype2) 
LDLIBS += -lGLEW -lGL -lSDL2 -lglimg $(shell pkg-config --libs freetype2) 
PROGRAM = gpu-real-time-shadows

default: $(PROGRAM)

$(PROGRAM): Animated.o Animation.o CatmullRom.o Control.o cpuImpl.o doubleBuffer.o edgeLookup.o Font.o fps_counter.o glObjects.o glUtil.o main.o modelLoader.o queryBuffer.o readFile.o Sampler.o Shader.o shaderLoader.o simplifyModel.o Texture.o vertexTypes.o
	$(CXX) -g $(CXXFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm *.o
	rm $(PROGRAM)

$PHONY: default clean
