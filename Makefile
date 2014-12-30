CXXFLAGS += --std=c++11 -DGLM_FORCE_RADIANS -g3
LDLIBS += -lGLEW -lGL -lSDL2 -lassimp
PROGRAM = gpu-real-time-shadows

default: $(PROGRAM)

$(PROGRAM): main.o glUtil.o modelLoader.o readFile.o shaderLoader.o Shader.o simplifyModel.o
	$(CXX) -g $(CXXFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm *.o
	rm $(PROGRAM)

$PHONY: default clean
