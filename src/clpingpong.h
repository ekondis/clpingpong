#pragma once

#include <memory>
#include <string>
#include <utility>
#define __CL_ENABLE_EXCEPTIONS
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <CL/cl.hpp>

class clPingPongApp {
private:
	cl::Platform pl;
	cl::Device dev;
	std::string buildLog;
	std::unique_ptr<cl::Program> program;
	std::unique_ptr<cl::CommandQueue> queue;
	std::unique_ptr<cl::Buffer> bufferDevAlloc, bufferHostAlloc;
	std::unique_ptr<cl::Kernel> kPingPong;
	void initialize_device(const int, const int);
	void create_resources(void);
	void verifyResult(cl_uint);
protected:
	std::pair<cl::Platform, cl::Device> selectPlatformDevice(const std::vector<int>&);
	void print_build_log(void);
	void resetBufferElements(void);
	cl_uint readBufferElement(const cl::Buffer&);
	cl_uint decrBufferElement(const cl::Buffer&);
	cl_uint decrBufferElementMapped(const cl::Buffer&);
	template<class hostF>
	double runExperiment(const int, const cl::Buffer&, hostF&);
public:
	clPingPongApp(const int, const int);
	void run(void);
};
