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

inline string trim(string str) {
	const string white_space_chars{" \n\r\0", 4};
	str.erase(0, str.find_first_not_of(white_space_chars));       //prefixing spaces
	str.erase(str.find_last_not_of(white_space_chars)+1);         //surfixing spaces
	return str; // use substring...
}

// in_Range template func

// Platform and device selection with a preference as hint
pair<cl::Platform, cl::Device> clPingPongApp::selectPlatformDevice(const vector<int> &preferences){
	VECTOR_CLASS<cl::Platform> platforms;
	VECTOR_CLASS<cl::Device> devices;
for(auto k:preferences){cout<<k <<',';}
	cout << "Platform/Device selection" << endl;
	cl::Platform::get(&platforms);
	cout << "Total platforms: " << platforms.size() << endl;
	string tmp;
	long int iP = 1, iD;
	for(VECTOR_CLASS<cl::Platform>::iterator pl = platforms.begin(); pl != platforms.end(); ++pl) {
		if( platforms.size() > 1 )
			cout << iP++ << ". ";
		cout << pl->getInfo<CL_PLATFORM_NAME>() << endl;
		pl->getDevices(CL_DEVICE_TYPE_ALL, &devices);
		iD = 1;
		for(VECTOR_CLASS<cl::Device>::iterator dev = devices.begin(); dev != devices.end(); ++dev)
			cout << '\t' << iD++ << ". " << trim(dev->getInfo<CL_DEVICE_NAME>()) << '/' << dev->getInfo<CL_DEVICE_VENDOR>() << endl;
	}
	// Platform selection
	if( platforms.size()>1 ){
		if( preferences.size()>0 && preferences[0]<=static_cast<int>(platforms.size()) ){
			iP = preferences[0];
		} else {
			cout << "Select platform index: ";
			cin >> iP;
			while( iP < 0 || iP > (int)platforms.size() ){
				cout << "Invalid platform index. Select again:";
				cin >> iP;
			}
		}
	} else
		iP = 1;
	cl::Platform platform = platforms[iP-1];
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	// Device selection
	if( devices.size()>1 ){
		if( preferences.size()>1 && preferences[1]<=static_cast<int>(devices.size()) ){
			iD = preferences[1];
		} else {
			cout << "Select device index: ";
			cin >> iD;
			while( iD<0 || iD>(int)devices.size() ){
				cout << "Invalid device index. Select again:";
				cin >> iD;
			}
		}
	} else
		iD = 1;
	cl::Device device = devices[static_cast<size_t>(iD-1)];
	return make_pair(platform, device);
}

void clPingPongApp::resetBufferElements(void){
	cl_uint *map = (cl_uint*)queue->enqueueMapBuffer(*bufferDevAlloc, CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_uint));
	map[0] = 0;
	queue->enqueueUnmapMemObject(*bufferDevAlloc, map);

	map = (cl_uint*)queue->enqueueMapBuffer(*bufferHostAlloc, CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_uint));
	map[0] = 0;
	queue->enqueueUnmapMemObject(*bufferHostAlloc, map);
}

cl_uint clPingPongApp::readBufferElement(const cl::Buffer& buffer){
	cl_uint *map = (cl_uint*)queue->enqueueMapBuffer(buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(cl_uint));
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
	cl_uint *map = (cl_uint*)queue->enqueueMapBuffer(buffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(cl_uint));
	cl_uint result = --map[0];
	queue->enqueueUnmapMemObject(buffer, map);
	return result;
}

void clPingPongApp::verifyResult(cl_uint result){
	constexpr unsigned int validResult = 0;
	if( result == validResult )
		cout << "PASSED!" << endl;
	else
		cout << "FAILED (" << result << "!=" << validResult << ")!" << endl;
}


void clPingPongApp::print_build_log(void){
    //string buildLog;
    //program.getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
    cout << "Build log:" << endl
        << " ******************** " << endl
        << '[' << buildLog << ']' << endl
        << " ******************** " << endl;
}

void clPingPongApp::initialize_device(const int preferred_plat_no, const int preferred_dev_no){
	//selectPlatformDevice(pl, dev);
	vector<int> preferences{};
	if( preferred_plat_no>=0 ){
		preferences.push_back(preferred_plat_no);
		if( preferred_dev_no>=0 ){
			preferences.push_back(preferred_dev_no);
		}
	}
	tie(pl, dev) = selectPlatformDevice(preferences);

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
	cl::Context context = cl::Context(VECTOR_CLASS<cl::Device>(1, dev));
	queue = unique_ptr<cl::CommandQueue>(new cl::CommandQueue(context, dev, 0/*CL_QUEUE_PROFILING_ENABLE*/));

	// Load and build kernel
	cl::Program::Sources src(1, make_pair(kernel_src, sizeof(kernel_src)-1/*remove NULL character*/));
	program = unique_ptr<cl::Program>(new cl::Program(context, src));

	try {
		program->build(VECTOR_CLASS<cl::Device>(1, dev), str_cl_parameters.c_str());
	} catch(cl::Error&){
		program->getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
		print_build_log();
		throw;
	}

	// Print built log if not empty
	//string buildLog; 
	program->getBuildInfo(dev, CL_PROGRAM_BUILD_LOG, &buildLog);
	buildLog = trim(buildLog);
	//cout << endl << "%%%%%%%%%" << buildLog.length() << ' ' << buildLog.empty()<< endl;
	if( !buildLog.empty() )
        print_build_log();
	//cout << endl << "%%%%%%%%%" << endl;

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

clPingPongApp::clPingPongApp(const int preferred_plat_no, const int preferred_dev_no){
	initialize_device(preferred_plat_no, preferred_dev_no);
}

void clPingPongApp::run(void){
	cout << "clpingpong - OpenCL host to device overhead evaluation" << endl << endl;

	create_resources();

	resetBufferElements();

	cout << "Experiment execution:" << endl;

	// Warm up
	cout << endl << "Warming up... " << flush;
	auto host_nop = []{};
	runExperiment(10000, *bufferDevAlloc, host_nop);
	queue->finish();
	cout << "OK" << endl;

	constexpr int LOOPS = 20000;

	// Device to device only experiments
	cout << endl << "Device <-> Device (w.sync)... " << std::flush;
	auto host_just_sync = [this]{ queue->finish(); };
	auto elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_just_sync);
	queue->finish();
	const auto loopTimeDev2DevD = elapsedTime/LOOPS*1000.;

	cout << "OK - Time: " << elapsedTime << " msecs";
	cout << " (" << loopTimeDev2DevD <<  " usecs/invocation)" << endl;

	cout << "Device <-> Device (w.sync)... " << std::flush;
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_just_sync);
	queue->finish();
	const auto loopTimeDev2DevH = elapsedTime/LOOPS*1000.;

	cout << "OK - Time: " << elapsedTime << " msecs";
	cout << " (" << loopTimeDev2DevH <<  " usecs/invocation)" << endl;

	// Device allocated with memory mapping experiment
	cout << "Experiment #1: Device allocated memory w. mapping:  " << std::flush;
	auto host_decr_mapped = [this]{ decrBufferElementMapped(*bufferDevAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_decr_mapped);
	const auto loopTimeDevAllocMapping = elapsedTime/LOOPS*1000.;

	// Verify result
	verifyResult( readBufferElement(*bufferDevAlloc) );

	// Device allocated with explicit transfer experiment
	cout << "Experiment #2: Device allocated memory w. transfer: " << std::flush;
	auto host_decr_transf = [this]{ decrBufferElement(*bufferDevAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferDevAlloc, host_decr_transf);
	const auto loopTimeDevAllocTransf = elapsedTime/LOOPS*1000.;

	// Verify result
	verifyResult( readBufferElement(*bufferDevAlloc) );

	// Host allocated with memory mapping experiment
	cout << "Experiment #3: Host allocated memory w. mapping:    " << std::flush;
	auto host_decr_mapped_halloc = [this]{ decrBufferElementMapped(*bufferHostAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_decr_mapped_halloc);
	const auto loopTimeHostAllocMapping = elapsedTime/LOOPS*1000.;

	// Verify result
	verifyResult( readBufferElement(*bufferHostAlloc) );

	// Host allocated with explicit transfer experiment
	cout << "Experiment #4: Host allocated memory w. transfer:   " << std::flush;
	auto host_decr_transf_halloc = [this]{ decrBufferElement(*bufferHostAlloc); };
	elapsedTime = runExperiment(LOOPS, *bufferHostAlloc, host_decr_transf_halloc);
	const auto loopTimeHostAllocTransf = elapsedTime/LOOPS*1000.;

	// Verify result
	verifyResult( readBufferElement(*bufferHostAlloc) );

	cout << endl;
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
