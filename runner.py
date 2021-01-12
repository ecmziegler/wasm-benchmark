#! /usr/bin/env python3

import os
import sys
import re
import json
import subprocess
from shlex import quote
from argparse import ArgumentParser
from yaml import load as yaml_load
try:
    from yaml import CLoader as YamlLoader
except ImportError:
    from yaml import Loader as YamlLoader
from urllib.parse import quote as urlquote

base_dir = os.path.dirname(os.path.abspath(__file__))

allowed_steps = {'build', 'run', 'analyze'}
native_envs = {'native'}
wasm_envs = {'d8', 'chrome', 'mozjs', 'firefox'}
allowed_envs = native_envs | wasm_envs
allowed_benchmarks = {'base64', 'zlib', 'box2d', 'lzma', 'micro', 'sqlite'}
whitespace = re.compile('\s')

class Analysis:
	class Event:
		def __init__ (self, line):
			fields = re.split(whitespace, line, 2)
			self.time = int(fields[0])
			self.event_id = fields[1]

		regex = re.compile('\\[EVENTS\\]\n')

	class Interval:
		def __init__ (self, line):
			fields = re.split(whitespace, line, 4)
			self.begin_time = int(fields[0])
			self.end_time = int(fields[1])
			self.interval_id = fields[2]
			self.numeric_id = int(fields[3])

		regex = re.compile('\\[INTERVALS\\]\n')

	class Progress:
		def __init__ (self, line):
			fields = re.split(whitespace, line, 2)
			self.time = int(fields[0])
			self.work = float(fields[1])
			self.performance = None

		regex = re.compile('\\[PROGRESS (.+)\\]\n')

	class Summary:
		def __init__ (self, json_string):
			fields = json.loads(json_string)
			self.start_up_time = fields['start_up_time']
			self.warm_up_time = fields['warm_up_time']
			self.effective_start_up_time = fields['effective_start_up_time']
			self.duration = fields['duration']
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
					progress.append(Analysis.Progress(line))
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
		
		# Compute performance
		for progress in self.progress.values():
			for index in range(len(progress)):
#				if index == 0:
#					progress[index].performance = progress[index].work / progress[index].time if progress[index].time > 0 else 0
#				else:
#					progress[index].performance = (progress[index].work - progress[index - 1].work) / (progress[index].time - progress[index - 1].time)
				if index == 0:
					progress[index].performance = progress[index + 1].work / progress[index + 1].time
				elif index < len(progress) - 1:
					progress[index].performance = (progress[index + 1].work - progress[index - 1].work) / (progress[index + 1].time - progress[index - 1].time)
				else:
					progress[index].performance = (progress[index].work - progress[index - 1].work) / (progress[index].time - progress[index - 1].time)
					

	def plot (self, axes, progress_id, scale, label, **kwargs):
		summary = self.summaries[progress_id]
		axes.plot([0, float(summary.start_up_time)/1000, float(summary.start_up_time + summary.warm_up_time)/1000, float(summary.duration)/1000], [0, 0, summary.peak_performance/scale, summary.peak_performance/scale], linestyle = 'dashed', **kwargs)
		axes.plot([float(progress.time)/1000 for progress in self.progress[progress_id]], [progress.performance/scale for progress in self.progress[progress_id]], linestyle = 'solid', label = label, **kwargs)

class Benchmark:
	class ExecutionProfile:
		def __init__ (self, benchmark_name, profile_name, config):
			self.name = profile_name
			self.quantity = config.get('quantity', profile_name)
			self.native_binary = os.path.join(base_dir, 'out', benchmark_name, 'native', config.get('binary', '{}_bench'.format(benchmark_name)))
			self.wasm_binary = os.path.join(base_dir, 'out', benchmark_name, 'wasm', config.get('binary', '{}_bench'.format(benchmark_name)))
			self.arguments = config.get('arguments', [])
			self.runs = config.get('runs', 1)

	def __init__ (self, name, envs, d8, node, mozjs):
		self.name = name
		self.d8 = d8
		self.node = node
		self.mozjs = mozjs
		with open(os.path.join('benchmarks', self.name, 'config.yaml'), 'r') as file:
			config = yaml_load(file, Loader = YamlLoader)
		if 'build' in config:
			build_config = config['build']
			self.configure = build_config.get('configure', ['cmake', os.path.join(os.pardir, os.pardir, os.pardir, 'benchmarks', self.name)])
			self.make = build_config.get('make', ['make'])
		else:
			self.configure = ['cmake', os.path.join(os.pardir, os.pardir, os.pardir, 'benchmarks', self.name)]
			self.make = ['make']
		if 'profiles' in config:
			self.profiles = [Benchmark.ExecutionProfile(self.name, profile_name, profile_config) for profile_name, profile_config in config['profiles'].items()]
		else:
			self.profiles = [Benchmark.ExecutionProfile(self.name, 'runs', {})]
		self.verbose = False
		self.run_profiler = False
		self.envs = set(envs)

	def set_verbose (self, enabled):
		self.verbose = enabled

	def set_run_profiler (self, enabled):
		self.run_profiler = enabled

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
					sys.stdout.write(out.decode("utf-8"))
					sys.stdout.flush()
				if err is not None:
					sys.stderr.write(err.decode("utf-8"))
					sys.stderr.flush()
				return proc.returncode

	@staticmethod
	def build_tools (envs, verbose = False):
		# Native build
		if not native_envs.isdisjoint(envs):
			build_dir = os.path.join(base_dir, 'out', 'tools', 'native')
			os.makedirs(build_dir, exist_ok = True)
			print('Building helper tools with Clang')
			subprocess.call(['cmake', os.path.join(base_dir, 'tools')], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)
			subprocess.call(['make'], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)

		# Wasm build
		if not wasm_envs.isdisjoint(envs):
			build_dir = os.path.join(base_dir, 'out', 'tools', 'wasm')
			os.makedirs(build_dir, exist_ok = True)
			print('Building helper tools with Emscripten')
			subprocess.call(['emcmake', 'cmake', os.path.join(base_dir, 'tools')], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)
			subprocess.call(['emmake', 'make'], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)

		# Chrome & Firefox dependencies
		if 'chrome' in envs or 'firefox' in envs:
			build_dir = os.path.join(base_dir, 'browser_support')
			print('Installing dependencies for Chrome/Firefox')
			subprocess.call(['npm', 'install'], cwd = build_dir, stdout = None if verbose else subprocess.DEVNULL)

	def build (self):
		# Native build
		if not native_envs.isdisjoint(self.envs):
			build_dir = os.path.join(base_dir, 'out', self.name, 'native')
			os.makedirs(build_dir, exist_ok = True)
			print('Building {benchmark} with Clang'.format(benchmark = self.name))
			return_code = self.call(self.configure, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)
			if return_code != 0:
				self.envs -= native_envs
			else:
				return_code = self.call(self.make, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL)
				if return_code != 0:
					self.envs -= native_envs

		# Wasm build
		if not wasm_envs.isdisjoint(self.envs):
			build_dir = os.path.join(base_dir, 'out', self.name, 'wasm')
			os.makedirs(build_dir, exist_ok = True)
			print('Building {benchmark} with Emscripten'.format(benchmark = self.name))
			return_code = self.call(['emcmake' if self.configure[0] == 'cmake' else 'emconfigure'] + self.configure, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL, stderr = None if self.verbose else subprocess.DEVNULL)
			if return_code != 0:
				self.envs -= wasm_envs
			else:
				return_code = self.call(['emmake'] + self.make, cwd = build_dir, stdout = None if self.verbose else subprocess.DEVNULL, stderr = None if self.verbose else subprocess.DEVNULL)
				if return_code != 0:
					self.envs -= wasm_envs
	
	def run (self):
		# Native execution
		if 'native' in self.envs:
			for profile in self.profiles:
				print('Benchmarking {benchmark} {profile} natively'.format(benchmark = self.name, profile = profile.name))
				args = [os.path.join(base_dir, 'out', 'tools', 'native', 'recorder'),
					'-r', str(profile.runs),
					'-R',
					'-o', os.path.join(base_dir, 'out', self.name, '{}_native.txt'.format(profile.name)),
					'--', profile.native_binary] + profile.arguments;
				if self.run_profiler:
					perf_output = os.path.join(base_dir, 'out', self.name, '{}_native.perf'.format(profile.name))
					if os.path.exists(perf_output):
						os.remove(perf_output)
					args[7:7] = [
						'perf',
						'record',
#						'-A',
						'-o', perf_output,
						'--'
					]
				if self.verbose:
					args.insert(1, '-v')
				return_code = self.call(args)
				if return_code != 0:
					sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
					sys.stderr.flush()
					self.envs.remove('native')

		# d8 execution
		if 'd8' in self.envs:
			for profile in self.profiles:
				print('Benchmarking {benchmark} {profile} in d8'.format(benchmark = self.name, profile = profile.name))
				with open(os.path.join(base_dir, 'out', self.name, '{}_d8.txt'.format(profile.name)), 'w') as output_file:
					cmd = [
						self.d8,
						'-e', '''const recorder_js = "{recorder}.mjs";
							 const wasm_js = "{module}.mjs";
							 const argv = {arguments};
							 const runs = {runs};
							 const verbose = {verbose};'''.format(
								recorder = os.path.join(base_dir, 'out', 'tools', 'wasm', 'recorder'),
								module = profile.wasm_binary,
								arguments = json.dumps(profile.arguments),
								runs = profile.runs,
								verbose = 'true' if self.verbose else 'false'),
						os.path.join(base_dir, 'wrapper.js')
					]
					if self.run_profiler:
						cmd[0:0] = [
							'perf',
							'record',
							'-k', 'mono',
							'-o', os.path.join(base_dir, 'out', self.name, '{}_d8.raw.perf'.format(profile.name)),
							'--'
						]
						cmd[8:8] = [
							'--perf-prof',
							'--no-wasm-async-compilation'
						]
					return_code = self.call(cmd, cwd = os.path.dirname(profile.wasm_binary), stdout = output_file)
				if return_code != 0:
					sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
					sys.stderr.flush()
					self.envs.remove('d8')
				if self.run_profiler:
					self.call([
						'perf',
						'inject',
						'-j',
						'-i', os.path.join(base_dir, 'out', self.name, '{}_d8.raw.perf'.format(profile.name)),
						'-o', os.path.join(base_dir, 'out', self.name, '{}_d8.perf'.format(profile.name))
					], cwd = os.path.dirname(profile.wasm_binary))

		# Chrome & Firefox execution
		for browser in 'chrome', 'firefox':
			if browser in self.envs:
				for profile in self.profiles:
					print('Benchmarking {benchmark} {profile} in {browser}'.format(benchmark = self.name, profile = profile.name, browser = browser.capitalize()))
					return_code = self.call([
						'node',
						'browser_support/run.js',
						browser,
						'wrapper.html?recorder=/{recorder}.mjs&wasm=/{module}.mjs&{arguments}&runs={runs}&verbose={verbose}'.format(
							recorder = urlquote(os.path.join('out', 'tools', 'wasm', 'recorder')),
							module = urlquote(os.path.relpath(profile.wasm_binary, base_dir)),
							arguments = '&'.join(['arg=' + urlquote(arg) for arg in profile.arguments]),
							runs = profile.runs,
							verbose = 'true' if self.verbose else 'false'),
						os.path.join(base_dir, 'out', self.name, '{profile}_{browser}.txt'.format(profile = profile.name, browser = browser))
					])
					if return_code != 0:
						sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
						sys.stderr.flush()
						self.envs.remove(browser)

		# Node execution
		if 'node' in self.envs:
			for profile in self.profiles:
				print('Benchmarking {benchmark} {profile} in Node.js'.format(benchmark = self.name, profile = profile.name))
				with open(os.path.join(base_dir, 'out', self.name, '{}_node.txt'.format(profile.name)), 'w') as output_file:
					return_code = self.call([
						self.node,
						'--experimental-modules',
						'--experimental-wasm-modules',
						os.path.join(base_dir, 'wrapper.js'),
						os.path.join(base_dir, 'out', 'tools', 'wasm', 'recorder'),
						profile.wasm_binary,
						str(profile.runs),
						'true' if self.verbose else 'false',
					] + profile.arguments, cwd = os.path.dirname(profile.wasm_binary), stdout = output_file)
				if return_code != 0:
					sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
					sys.stderr.flush()
					self.envs.remove('node')

		# mozjs execution
		if 'mozjs' in self.envs:
			for profile in self.profiles:
				print('Benchmarking {benchmark} {profile} in SpiderMonkey'.format(benchmark = self.name, profile = profile.name))
				with open(os.path.join(base_dir, 'out', self.name, '{}_mozjs.txt'.format(profile.name)), 'w') as output_file:
					return_code = self.call([
						self.mozjs,
						'-e', '''const recorder_js = "{recorder}.mjs";
							 const wasm_js = "{module}.mjs";
							 const argv = {arguments};
							 const runs = {runs};
							 const verbose = {verbose};'''.format(
								recorder = os.path.join(base_dir, 'out', 'tools', 'wasm', 'recorder'),
								module = profile.wasm_binary,
								arguments = json.dumps(profile.arguments),
								runs = profile.runs,
								verbose = 'true' if self.verbose else 'false'),
						'-f', os.path.join(base_dir, 'wrapper.js')
					], cwd = os.path.dirname(profile.wasm_binary), stdout = output_file)
				if return_code != 0:
					sys.stderr.write('Execution failed with status {status}\n'.format(status = return_code))
					sys.stderr.flush()
					self.envs.remove('mozjs')

	def analyze (self, format):
		performances_figure = plt.figure()
		performances_figure.set_tight_layout(True)
		performances_axes = performances_figure.add_subplot()
		performances_axes.set_title('{benchmark} performance'.format(benchmark = self.name))
		
		times_figure = plt.figure()
		times_figure.set_tight_layout(True)
		times_axes = times_figure.add_subplot()
		times_axes.set_title('{benchmark} times'.format(benchmark = self.name))
		summary_positions = []
		base_performances = []
		additional_performances = []
		start_up_times = []
		warm_up_times = []
		summary_colors = []
		summary_ticks = []
		summary_labels = []
		summary_legend_labels = {env: color for env, color in {
				'native': 'gray',
				'd8': 'cornflowerblue',
				'chrome': 'lightsteelblue',
				'node': 'darkorange',
				'mozjs': 'coral',
				'firefox': 'crimson'
			}.items() if env in self.envs}
		position = 0
		with open(os.path.join(base_dir, 'out', self.name, 'overview.html'), 'w') as overview:
			overview.write('<html>\n<head>\n\t<title>{benchmark} benchmark</title>\n</head><body>\n'.format(benchmark = self.name))
		
			for profile in self.profiles:
				print('Analyzing {benchmark} {profile}'.format(benchmark = self.name, profile = profile.name))
				progress_figure = plt.figure()
				progress_figure.set_tight_layout(True)
				progress_axes = progress_figure.add_subplot()
				progress_axes.set_title('{benchmark} {profile}'.format(benchmark = self.name, profile = profile.name))
				scale = 1
				summary_ticks.append(position)
				summary_labels.append(profile.name)

				# Native execution
				if 'native' in self.envs:
					with open(os.path.join(base_dir, 'out', self.name, '{}_native.txt'.format(profile.name)), 'r') as file:
						analysis = Analysis(file)
					summary = analysis.summaries[profile.name]
					scale = summary.peak_performance
					base_performances.append(summary.peak_performance * (1.0 - summary.effective_start_up_time / summary.duration) / scale)
					additional_performances.append(summary.peak_performance / scale - base_performances[-1])
					start_up_times.append(summary.start_up_time/1000)
					warm_up_times.append(summary.warm_up_time/1000)
					summary_colors.append('gray')
					summary_positions.append(position)
					position += 1
					summary_legend_labels['native'] = 'gray'
					analysis.plot(progress_axes, profile.quantity, scale, 'native', color = 'gray')

				# Other executions
				event_axis_shift = 0.0
				for env in self.envs:
					if env == 'native':
						continue
					with open(os.path.join(base_dir, 'out', self.name, '{profile}_{env}.txt'.format(profile = profile.name, env = env)), 'r') as file:
						analysis = Analysis(file)
					summary = analysis.summaries[profile.name]
					base_performances.append(summary.peak_performance * (1.0 - summary.effective_start_up_time / summary.duration) / scale)
					additional_performances.append(summary.peak_performance / scale - base_performances[-1])
					start_up_times.append(summary.start_up_time/1000)
					warm_up_times.append(summary.warm_up_time/1000)
					summary_colors.append(summary_legend_labels[env])
					summary_positions.append(position)
					position += 1
					analysis.plot(progress_axes, profile.quantity, scale, env, color = summary_legend_labels[env])
					
#					if len(analysis.events) > 0:
#						event_axis_shift -= 0.2;
#						events = progress_axes.secondary_xaxis(event_axis_shift)
#						events.set_xlabel('{} events'.format(env))
#						events.set_xticks([event.time/1000 for event in analysis.events])
#						events.set_xticklabels([event.event_id for event in analysis.events])
					
#					if len(analysis.intervals) > 0:
#						for interval in analysis.intervals:
#							progress_axes.hlines(0.1, interval.begin_time/1000, interval.end_time/1000, lw = 5)
#							progress_axes.text((interval.begin_time + interval.end_time)/2000, 0.2, interval.interval_id, horizontalalignment = 'center', verticalalignment = 'bottom')
						

				progress_axes.set_xlim(xmin = 0)
				progress_axes.set_ylim(ymin = 0)
				progress_axes.set_xlabel('Execution time [ms]')
				progress_axes.set_ylabel('Normalized performance')
				progress_axes.legend(loc = 'lower right')
				with open(os.path.join(base_dir, 'out', self.name, '{profile}.{format}'.format(profile = profile.name, format = format)), 'w') as file:
					progress_figure.savefig(file, format = format)
				plt.close(progress_figure)
				
				overview.write('\t<img src="{}">\n'.format(os.path.join(base_dir, 'out', self.name, '{profile}.{format}'.format(profile = profile.name, format = format))))
				
				summary_ticks[-1] = (summary_ticks[-1] + position - 1) / 2
				position += 1

			print('Generating {benchmark} summary and overview'.format(benchmark = self.name))

			performances_axes.bar(summary_positions, base_performances, 1, color = summary_colors)
			performances_axes.bar(summary_positions, additional_performances, 1, bottom = base_performances, color = summary_colors, edgecolor = 'white', hatch = '//')
			performances_axes.set_ylim(ymin = 0)
			performances_axes.set_xticks(summary_ticks)
			performances_axes.set_xticklabels(summary_labels)
			performances_axes.set_ylabel('Normalized performance')
			performances_axes.legend(handles = [matplotlib.patches.Patch(facecolor = color, label = label) for label, color in summary_legend_labels.items()], loc = 'lower right')
			with open(os.path.join(base_dir, 'out', self.name, 'performance.{format}'.format(format = format)), 'w') as file:
				performances_figure.savefig(file, format = format)
			plt.close(performances_figure)

			times_axes.bar(summary_positions, start_up_times, 1, color = summary_colors)
			times_axes.bar(summary_positions, warm_up_times, 1, bottom = start_up_times, color = summary_colors, edgecolor = 'white', hatch = '//')
			times_axes.set_ylim(ymin = 0)
			times_axes.set_xticks(summary_ticks)
			times_axes.set_xticklabels(summary_labels)
			times_axes.set_ylabel('Start/warm up time [ms]')
			times_axes.legend(handles = [matplotlib.patches.Patch(facecolor = color, label = label) for label, color in summary_legend_labels.items()], loc = 'lower right')
			with open(os.path.join(base_dir, 'out', self.name, 'times.{format}'.format(format = format)), 'w') as file:
				times_figure.savefig(file, format = format)
			plt.close(times_figure)

			overview.write('\t<img src="{performance}">\n\t<img src="{times}">\n</body>\n</html>\n'.format(performance = os.path.join(base_dir, 'out', self.name, 'performance.{format}'.format(format = format)), times = os.path.join(base_dir, 'out', self.name, 'times.{format}'.format(format = format))))

if __name__ == '__main__':
	parser = ArgumentParser()
	parser.add_argument('--verbose', '-v', default = False, action = 'store_true', help = 'Print executed commands (default: false)')
	parser.add_argument('--perf', '-p', default = False, action = 'store_true', help = 'Run perforkance profiler during native and d8 benchmark execution (default: false)')
	parser.add_argument('--step', '-s', type = str, action = 'append', choices = allowed_steps, default = [], help = 'Step to execute (default: build run analyze)')
	parser.add_argument('--env', '-e', type = str, action = 'append', choices = allowed_envs, default = [], help = 'Environments to benchmark (default all)')
	parser.add_argument('--format','-f', type = str, default = 'svg', choices = ['svg'], help = 'Output format for analysis (default: svg)')
	parser.add_argument('--d8', type = str, default = 'd8', help = 'Path to V8 shell (default: d8)')
	parser.add_argument('--node', type = str, default = 'node', help = 'Path to Node.js (default: node)')
	parser.add_argument('--mozjs', type = str, default = 'js', help = 'Path to SpiderMonkey shell (default: js)')
	parser.add_argument('benchmarks', metavar = '<benchmark>', type = str, choices = allowed_benchmarks, default = allowed_benchmarks, nargs = '*', help = 'The name(s) of the benchmark(s) to run')
	args = parser.parse_args()
	import matplotlib
	matplotlib.use(args.format)
	import matplotlib.pyplot as plt
	if len(args.step) == 0:
		args.step = allowed_steps
	if len(args.env) == 0:
		args.env = allowed_envs
	if 'build' in args.step:
		Benchmark.build_tools(args.env, args.verbose)
	for name in args.benchmarks:
		#try:
			benchmark = Benchmark(name, args.env, args.d8, args.node, args.mozjs)
			benchmark.set_verbose(args.verbose)
			benchmark.set_run_profiler(args.perf)
			if 'build' in args.step:
				benchmark.build()
			if 'run' in args.step:
				benchmark.run()
			if 'analyze' in args.step:
				benchmark.analyze(args.format)
		#except FileNotFoundError:
		#	sys.stderr.write('Skipping {benchmark}, config file not found or erroneous.\n'.format(benchmark = name))
		#	sys.stderr.flush()
