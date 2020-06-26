#! /usr/bin/env python3

import os
import sys
import re
import json
import subprocess
import matplotlib
matplotlib.use('SVG')
import matplotlib.pyplot as plt
from shlex import quote
from argparse import ArgumentParser
from yaml import load as yaml_load
try:
    from yaml import CLoader as YamlLoader
except ImportError:
    from yaml import Loader as YamlLoader

base_dir = os.path.dirname(os.path.abspath(__file__))

class Analysis:
	class Event:
		def __init__ (self, line):
			fields = line.split('\t')
			self.time = int(fields[0])
			self.event_id = fields[1]

		regex = re.compile('\\[EVENTS\\]\n')

	class Interval:
		def __init__ (self, line):
			fields = line.split('\t')
			self.start_time = int(fields[0])
			self.stop_time = int(fields[1])
			self.interval_id = fields[2]
			self.numeric_id = int(fields[3])

		regex = re.compile('\\[INTERVALS\\]\n')

	class Progress:
		def __init__ (self, line):
			fields = line.split('\t')
			self.time = int(fields[0])
			self.work = float(fields[1])

		regex = re.compile('\\[PROGRESS (.+)\\]\n')

	class Summary:
		def __init__ (self, json_string):
			fields = json.loads(json_string)
			self.start_up_time = fields['start_up_time']
			self.ramp_up_time = fields['ramp_up_time']
			self.effective_start_up_time = fields['effective_start_up_time']
			self.initial_performance = fields['initial_performance']
			self.peak_performance = fields['peak_performance']

	def __init__ (self, input_file):
		self.events = []
		self.intervals = []
		self.progress = {}
		self.summaries = {}

		# Read events
		assert Analysis.Event.regex.match(input_file.readline()) is not None
		for line in input_file:
			line = line.strip('\n')
			if len(line) == 0:
				break
			else:
				self.events.append(Analysis.Event(line))

		# Read intervals
		assert Analysis.Interval.regex.match(input_file.readline()) is not None
		for line in input_file:
			line = line.strip('\n')
			if len(line) == 0:
				break
			else:
				self.intervals.append(Analysis.Interval(line))

		for line in input_file:
			# Read progress
			match = Analysis.Progress.regex.match(line)
			assert match is not None
			progress = []		
			for line in input_file:
				line = line.strip('\n')
				if len(line) == 0:
					break
				else:
					self.intervals.append(Analysis.Progress(line))
			self.progress[match.group(1)] = progress

			# Read summary
			json_string = ""
			for line in input_file:
				line = line.strip('\n')
				if len(line) == 0:
					break
				else:
					json_string += line
			self.summaries[match.group(1)] = Analysis.Summary(json_string)

	def plot (self, axes, progress_id):
		axes.plot([progress.time for progress in self.progress[progress_id]], [progress.work for progress in self.progress[progress_id]])

class Benchmark:
	def __init__ (self, name, envs, d8):
		self.name = name
		self.d8 = d8
		with open(os.path.join('benchmarks', self.name, 'config.yaml'), 'r') as file:
			config = yaml_load(file, Loader = YamlLoader)
		if 'build' in config:
			build_config = config['build']
			self.configure = build_config.get('configure', ['cmake', os.path.join(os.pardir, os.pardir, os.pardir, 'benchmarks', self.name)])
			self.make = build_config.get('make', ['make'])
			self.native_binary = os.path.join(base_dir, 'out', self.name, 'native', build_config.get('binary', '{}_bench'.format(self.name)))
			self.wasm_binary = os.path.join(base_dir, 'out', self.name, 'wasm', build_config.get('binary', '{}_bench'.format(self.name)))
		if 'execution' in config:
			execution_config = config['execution']
			self.runs = execution_config.get('runs', 1)
		self.verbose = False
		self.envs = envs

	def set_verbose (self, enabled):
		self.verbose = enabled

	def call (self, arguments, cwd = None, stdout = None, stderr = None):
		if self.verbose:
			sys.stdout.write(' '.join(quote(argument) for argument in arguments))
			sys.stdout.write('\n')
			sys.stdout.flush()
		if stdout is None and stderr is None:
			return subprocess.call(arguments, cwd = cwd)
		else:
			with subprocess.Popen(arguments, cwd = cwd, stdout = stdout, stderr = stderr) as proc:
				try:
					out, err = proc.communicate(timeout=60)
				except subprocess.TimeoutExpired:
					proc.kill()
					out, err = proc.communicate()
					sys.stderr.write('Timeout\n')
				if out is not None:
					sys.stdout.write(out)
					sys.stdout.flush()
				if err is not None:
					sys.stderr.write(out)
					sys.stderr.flush()
				return proc.returncode

	@staticmethod
	def build_tools (envs, verbose = False):
		# Native build
		if 'native' in envs:
			build_dir = os.path.join(base_dir, 'out', 'tools', 'native')
			os.makedirs(build_dir, exist_ok = True)
			print('Building helper tools with Clang')
			subprocess.call(['cmake', os.path.join(base_dir, 'tools')], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)
			subprocess.call(['make'], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)

		# Wasm build
		if 'd8' in envs:
			build_dir = os.path.join(base_dir, 'out', 'tools', 'wasm')
			os.makedirs(build_dir, exist_ok = True)
			print('Building helper tools with Emscripten')
			subprocess.call(['emcmake', 'cmake', os.path.join(base_dir, 'tools')], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)
			subprocess.call(['emmake', 'make'], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)

	def build (self):
		# Native build
		if 'native' in self.envs:
			build_dir = os.path.dirname(self.native_binary)
			os.makedirs(build_dir, exist_ok = True)
			print('Building {name} with Clang'.format(name = self.name))
			self.call(self.configure, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)
			self.call(self.make, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)

		# Wasm build
		if 'd8' in self.envs:
			build_dir = os.path.dirname(self.wasm_binary)
			os.makedirs(os.path.dirname(self.wasm_binary), exist_ok = True)
			print('Building {name} with Emscripten'.format(name = self.name))
			self.call(['emcmake' if self.configure[0] == 'cmake' else 'emconfigure'] + self.configure, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)
			self.call(['emmake'] + self.make, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)
	
	def run (self):
		# Native execution
		if 'native' in self.envs:
			print('Benchmarking {name} natively'.format(name = self.name))
			args = [os.path.join(base_dir, 'out', 'tools', 'native', 'recorder'), '-r', str(self.runs), '-R', '-o', os.path.join(base_dir, 'out', self.name, 'benchmark_native.txt'), '--', self.native_binary];
			if self.verbose:
				args.insert(1, '-v')
			return_code = self.call(args)
			if return_code != 0:
				sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
				sys.stderr.flush()

		# d8 execution
		if 'd8' in self.envs:
			print('Benchmarking {name} in d8'.format(name = self.name))
			with open(os.path.join(base_dir, 'out', self.name, 'benchmark_d8.txt'), 'w') as output_file:
				return_code = self.call([
					self.d8,
					'-e', 'const recorder_js = "{recorder}.mjs"; const wasm_js = "{module}.mjs"; const runs = {runs}; const verbose = {verbose};'.format(recorder = os.path.join(base_dir, 'out', 'tools', 'wasm', 'recorder'), module = self.wasm_binary, runs = self.runs, verbose = 'true' if self.verbose else 'false'),
					os.path.join(base_dir, 'd8_wrapper.js')
				], cwd = os.path.dirname(self.wasm_binary), stdout = output_file)
			if return_code != 0:
				sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
				sys.stderr.flush()

	def analyze (self):
		print('Analyzing {name}'.format(name = self.name))

		# Native execution
		if 'native' in self.envs:
			with open(os.path.join(base_dir, 'out', self.name, 'benchmark_native.txt'), 'r') as file:
				analysis = Analysis(file)
			figure = plt.figure()
			axes = figure.add_subplot()
			axes.set_title('{name} native execution'.format(name = self.name))
			analysis.plot(axes, 'encode')
			with open(os.path.join(base_dir, 'out', self.name, 'benchmark_native.svg'), 'w') as file:
				figure.savefig(file)

		# d8 execution
		if 'd8' in self.envs:
			pass

if __name__ == '__main__':
	parser = ArgumentParser()
	parser.add_argument('--verbose', '-v', default = False, action = 'store_true', help = 'Print executed commands (default: false)')
	parser.add_argument('--step', '-s', type = str, action = 'append', default = [], help = 'Step to execute (default: build run analyze)')
	parser.add_argument('--env', '-e', type = str, action = 'append', default = [], help = 'Environments to benchmark (default all)')
	parser.add_argument('--d8', type = str, default = 'd8', help = 'Path to d8 (default: d8)')
	parser.add_argument('benchmarks', metavar = '<benchmark>', type = str, default = ['box2d', 'coremark', 'lzma', 'sqlite', 'zlib'], nargs = '*', help = 'The name(s) of the benchmark(s) to run')
	args = parser.parse_args()
	if len(args.step) == 0:
		args.step = ['build', 'run', 'analyze']
	if len(args.env) == 0:
		args.env = ['native', 'd8']
	if 'build' in args.step:
		Benchmark.build_tools(args.env, args.verbose)
	for name in args.benchmarks:
		try:
			benchmark = Benchmark(name, args.env, args.d8)
			benchmark.set_verbose(args.verbose)
			if 'build' in args.step:
				benchmark.build()
			if 'run' in args.step:
				benchmark.run()
			if 'analyze' in args.step:
				benchmark.analyze()
		except FileNotFoundError:
			sys.stderr.write('Skipping {name}, config file not found or erroneous.\n'.format(name = name))
			sys.stderr.flush()
