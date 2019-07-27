#pragma once

#include <memory>
#include <string>
#include <utility>
#define __CL_ENABLE_EXCEPTIONS
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <CL/cl.hpp>

class clPingPongApp {
private:
	using device_vector = std::vector<cl::Device>;
	std::vector<cl::Platform> hostPlatforms;
	std::vector<device_vector> hostDevices;
	std::pair<int, int> preference;
	cl::Platform pl;
	cl::Device dev;
	std::string buildLog;
	std::unique_ptr<cl::Program> program;
	std::unique_ptr<cl::CommandQueue> queue;
	std::unique_ptr<cl::Buffer> bufferDevAlloc, bufferHostAlloc;
	std::unique_ptr<cl::Kernel> kPingPong;
	void enumerate_devices(void);
	void choose_device(void);
	void create_resources(void);
	void verifyResult(cl_uint);
protected:
	std::pair<cl::Platform, cl::Device> selectPlatformDevice(void);
	void print_build_log(void);
	void resetBufferElements(void);
	cl_uint readBufferElement(const cl::Buffer&);
	cl_uint decrBufferElement(const cl::Buffer&);
	cl_uint decrBufferElementMapped(const cl::Buffer&);
	template<class hostF>
	double runExperiment(const int, const cl::Buffer&, hostF&);
public:
	clPingPongApp(const int=-1, const int=-1);
	void run(void);
};
