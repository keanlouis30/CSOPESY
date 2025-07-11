#pragma once
#include <string>
#include <fstream>
#include <iostream>

class Config
{
public:
	int num_cpu;
	std::string scheduler;
	int quantum_cycles;
	int batch_process_freq;
	int min_ins;
	int max_ins;
	int delays_per_exec;
	int max_overall_mem;
	int mem_per_frame;
	int mem_per_proc;

	Config() : num_cpu(1), scheduler("fcfs"), quantum_cycles(1), batch_process_freq(1), min_ins(1), max_ins(1), delays_per_exec(0), max_overall_mem(16384), mem_per_frame(16), mem_per_proc(4096) {}

	bool loadFromFile(const std::string &filename)
	{
		std::ifstream file(filename);

		if (!file.is_open())
		{
			std::cout << "Error: Could not open " << filename << std::endl;
			return false;
		}

		file >> num_cpu;
		file >> scheduler;
		file >> quantum_cycles;
		file >> batch_process_freq;
		file >> min_ins;
		file >> max_ins;
		file >> delays_per_exec;
		file >> max_overall_mem;
		file >> mem_per_frame;
		file >> mem_per_proc;

		if (file.fail())
		{
			std::cout << "Error: Invalid format in config file" << std::endl;
			file.close();
			return false;
		}

		file.close();
		return true;
	}
};