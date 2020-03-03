#define NOMINMAX
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <tuple>
#include <algorithm>
#include "clpingpong.h"

using namespace std;

static const char kernel_src[] = (
#include "kernels.h"
);

string trim(string str) {
	const string white_space_chars{" \n\r\0", 4};
	str.erase(0, str.find_first_not_of(white_space_chars));       //prefixing spaces
	str.erase(str.find_last_not_of(white_space_chars)+1);         //surfixing spaces
	return str; // use substring...
}

template<typename T>
bool in_range(const T &v, const T &low, const T &high){
	if( v<low )
		return false;
	else if( v>high )
		return false;
	return true;
}

// Platform and device selection with a preference as hint
pair<cl::Platform, cl::Device> clPingPongApp::selectPlatformDevice(void) const{
	cout << "Platform/Device selection" << endl;
	cout << "Total platforms: " << hostPlatforms.size() << endl;
	string tmp;
	long int iP = 1, iD;
	for(const auto &pl:hostPlatforms) {
		if( hostPlatforms.size() > 1 )
			cout << iP << ". ";
		cout << pl.getInfo<CL_PLATFORM_NAME>() << endl;
		iD = 1;
		for(const auto &dev:hostDevices[iP-1])
			cout << '\t' << iD++ << ". " << trim(dev.getInfo<CL_DEVICE_NAME>()) << '/' << dev.getInfo<CL_DEVICE_VENDOR>() << endl;
		iP++;
	}
	// Platform selection
	if( hostPlatforms.size()>1 ){
		if( in_range<int>(preference.first, 1, static_cast<int>(hostPlatforms.size())) ){
			iP = preference.first;
		} else {
			cout << "Select platform index: ";
			cin >> iP;
			while( iP < 0 || iP > static_cast<int>(hostPlatforms.size()) ){
				cout << "Invalid platform index. Select again:";
				cin >> iP;
			}
		}
	} else
		iP = 1;
	const auto platform = hostPlatforms[iP-1];
	auto &devices = hostDevices[iP-1];
	// Device selection
	if( devices.size()>1 ){
		if( in_range<int>(preference.second, 1, static_cast<int>(devices.size())) ){
			iD = preference.second;
		} else {
			cout << "Select device index: ";
			cin >> iD;
			while( iD<0 || iD>static_cast<int>(devices.size()) ){
				cout << "Invalid device index. Select again:";
				cin >> iD;
			}
		}
	} else
		iD = 1;
	const auto device = devices[static_cast<size_t>(iD-1)];
	return make_pair(platform, device);
}

void clPingPongApp::resetBufferElements(void){
	cl_uint *map = reinterpret_cast<cl_uint*>(queue->enqueueMapBuffer(*bufferDevAlloc, CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_uint)));
	map[0] = 0;
	queue->enqueueUnmapMemObject(*bufferDevAlloc, map);

	map = reinterpret_cast<cl_uint*>(queue->enqueueMapBuffer(*bufferHostAlloc, CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_uint)));
	map[0] = 0;
	queue->enqueueUnmapMemObject(*bufferHostAlloc, map);
}

cl_uint clPingPongApp::readBufferElement(const cl::Buffer& buffer) const{
	cl_uint *map = reinterpret_cast<cl_uint*>(queue->enqueueMapBuffer(buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(cl_uint)));
	cl_uint result = map[0];
	queue->enqueueUnmapMemObject(buffer, map);
	return result;
}

cl_uint clPingPongApp::decrBufferElement(const cl::Buffer& buffer){
	cl_uint element;
	queue->enqueueReadBuffer(buffer, CL_TRUE, 0, sizeof(cl_uint), &element);
	cl_uint result = --element;
	queue->enqueueWriteBuffer(buffer, CL_TRUE, 0, sizeof(cl_uint), &element);
	return result;
}

cl_uint clPingPongApp::decrBufferElementMapped(const cl::Buffer& buffer){
	cl_uint *map = reinterpret_cast<cl_uint*>(queue->enqueueMapBuffer(buffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(cl_uint)));
	cl_uint result = --map[0];
	queue->enqueueUnmapMemObject(buffer, map);
	return result;
}

void clPingPongApp::verifyResult(cl_uint result) const{
	constexpr unsigned int validResult = 0;
	if( result == validResult )
		cout << "PASSED!" << endl;
	else
		cout << "FAILED (" << result << "!=" << validResult << ")!" << endl;
}


void clPingPongApp::print_build_log(void) const{
    //string buildLog;
    //program.getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
    cout << "Build log:" << endl
        << " ******************** " << endl
        << '[' << buildLog << ']' << endl
        << " ******************** " << endl;
}

void clPingPongApp::enumerate_devices(void){
	cl::Platform::get(&hostPlatforms);
	for(const auto &p:hostPlatforms) {
		VECTOR_CLASS<cl::Device> devices;
		p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
		hostDevices.push_back( devices );
	}
}

void clPingPongApp::choose_device(void){
	tie(pl, dev) = selectPlatformDevice();

	cl_device_type devType;
	dev.getInfo(CL_DEVICE_TYPE, &devType);

	cout << endl << "Device info" << endl;
	string sInfo;

	dev.getInfo(CL_DEVICE_NAME, &sInfo);
	cout << "Device:         " << trim(sInfo) << endl;
	dev.getInfo(CL_DRIVER_VERSION, &sInfo);
	cout << "Driver version: " << trim(sInfo) << endl;
}

void clPingPongApp::create_resources(void){
	const string str_cl_parameters{"-Werror -cl-std=CL1.2"};

	// Create context, queue
	cl::Context context{VECTOR_CLASS<cl::Device>(1, dev)};
	queue = unique_ptr<cl::CommandQueue>(new cl::CommandQueue(context, dev, 0));

	// Load and build kernel
	cl::Program::Sources src{1, make_pair(kernel_src, sizeof(kernel_src)-1/*remove NULL character*/)};
	program = unique_ptr<cl::Program>(new cl::Program(context, src));

	try {
		program->build(VECTOR_CLASS<cl::Device>(1, dev), str_cl_parameters.c_str());
	} catch(cl::Error&){
		program->getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
		print_build_log();
		throw;
	}

	// Print built log if not empty
	program->getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
	buildLog = trim(buildLog);
	if( !buildLog.empty() )
        print_build_log();

	// Create buffer
	bufferDevAlloc = unique_ptr<cl::Buffer>(new cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_int)));
	bufferHostAlloc = unique_ptr<cl::Buffer>(new cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(cl_int)));

	// Create kernel
	kPingPong = unique_ptr<cl::Kernel>(new cl::Kernel(*program, "kPingPong"));
}

template<class hostF>
double clPingPongApp::runExperiment(const int loops, const cl::Buffer& buffer, hostF& host_func){
	resetBufferElements();
	auto clk_begin = chrono::high_resolution_clock::now();
	for(int iter=0; iter<loops; iter++) {
		host_func();
		kPingPong->setArg(0, buffer);
		queue->enqueueNDRangeKernel(*kPingPong, cl::NullRange, 1, 1, nullptr, nullptr);
	}
	queue->finish();
	auto clk_end = chrono::high_resolution_clock::now();
	return static_cast<double>( chrono::duration_cast<chrono::nanoseconds>(clk_end-clk_begin).count() )/1000000.0;
}

clPingPongApp::clPingPongApp(const int preferred_plat_no, const int preferred_dev_no):preference{preferred_plat_no, preferred_dev_no}{
	enumerate_devices();
}

void clPingPongApp::run(void){
	cout << "clPingPong - OpenCL host to device overhead evaluation" << endl << endl;

	choose_device();
	create_resources();

	resetBufferElements();

	// Warm up
	cout << endl << "Warming up... " << flush;
	auto host_nop = []{};
	runExperiment(10000, *bufferDevAlloc, host_nop);
	queue->finish();
	cout << "OK" << endl << endl;

	constexpr int LOOPS = 20000;

	// Device to device only experiments
	cout << "Device <-> Device reference experiments" << endl;

	cout << "Device allocated memory w.sync: " << std::flush;
	auto host_just_sync = [this]{ queue->finish(); };
	auto elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_just_sync);
	queue->finish();
	const auto loopTimeDev2DevD = elapsedTime/LOOPS*1000.;

	cout << elapsedTime << " msecs";
	cout << " (" << loopTimeDev2DevD <<  " usecs/invocation)" << endl;

	cout << "Host allocated memory w.sync:   " << std::flush;
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_just_sync);
	queue->finish();
	const auto loopTimeDev2DevH = elapsedTime/LOOPS*1000.;

	cout << elapsedTime << " msecs";
	cout << " (" << loopTimeDev2DevH <<  " usecs/invocation)" << endl << endl;

	// Host to device only experiments
	cout << "Host <-> Device experiments" << endl;

	// Device allocated with memory mapping experiment
	cout << "Experiment #1: Device allocated memory w. mapping:  " << std::flush;
	auto host_decr_mapped = [this]{ decrBufferElementMapped(*bufferDevAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_decr_mapped);
	const auto loopTimeDevAllocMapping = elapsedTime/LOOPS*1000.;
	verifyResult( readBufferElement(*bufferDevAlloc) );

	// Device allocated with explicit transfer experiment
	cout << "Experiment #2: Device allocated memory w. transfer: " << std::flush;
	auto host_decr_transf = [this]{ decrBufferElement(*bufferDevAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_decr_transf);
	const auto loopTimeDevAllocTransf = elapsedTime/LOOPS*1000.;
	verifyResult( readBufferElement(*bufferDevAlloc) );

	// Host allocated with memory mapping experiment
	cout << "Experiment #3: Host allocated memory w. mapping:    " << std::flush;
	auto host_decr_mapped_halloc = [this]{ decrBufferElementMapped(*bufferHostAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_decr_mapped_halloc);
	const auto loopTimeHostAllocMapping = elapsedTime/LOOPS*1000.;
	verifyResult( readBufferElement(*bufferHostAlloc) );

	// Host allocated with explicit transfer experiment
	cout << "Experiment #4: Host allocated memory w. transfer:   " << std::flush;
	auto host_decr_transf_halloc = [this]{ decrBufferElement(*bufferHostAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_decr_transf_halloc);
	const auto loopTimeHostAllocTransf = elapsedTime/LOOPS*1000.;
	verifyResult( readBufferElement(*bufferHostAlloc) );

	cout << endl << "Benchmark summary (Host <-> Device)" << endl;
	cout << "                        Time measurements (usecs/loop)" << endl;
	cout << "Communication type  |  Regular buffer  |  Host allocated  |" << endl;
	cout << "--------------------|------------------|------------------|" << endl;
	cout << fixed << setprecision(4) <<
		"Memory mapped       |  " << setw(14) << loopTimeDevAllocMapping << "  |  " << setw(14) << loopTimeHostAllocMapping << "  |" << endl <<
		"Explicit transfer   |  " << setw(14) << loopTimeDevAllocTransf << "  |  " << setw(14) << loopTimeHostAllocTransf << "  |" << endl;

	const auto minTime = min({loopTimeDevAllocMapping, loopTimeDevAllocTransf, loopTimeHostAllocMapping, loopTimeHostAllocTransf});
	const auto loopTimeDev2Dev = min(loopTimeDev2DevD, loopTimeDev2DevH);
	cout << endl << "Host overhead: " << (100.*(minTime-loopTimeDev2Dev))/loopTimeDev2Dev <<  "%" << endl;
}
