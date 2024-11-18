import ctypes
import datetime
import threading
import time
import os
import glob
import ws_server as ws
import json


buffers = {}
buffer_size = 1 * 1024 * 1024  # 1 MB
buffer_mutex = threading.Lock()

ignore_files = [
	'logs/code_mem_log.txt',
	'logs/mem_log.txt',
	'logs/main.log',
]


print_callback_t = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p)


def print_logs(filename, logstring):
	global buffer_mutex, stop_thread
	encoding = 'utf-8'
	filename = str(filename.decode(encoding))
	logstring = str(logstring.decode(encoding))
	if filename in ignore_files:
		return
	if filename == "logs/video_mem_log.txt" and logstring.startswith("VIDEO_BUF"):
		data = json.dumps({'text': logstring[11:]})	# {'text': ""}
		# print_logs(str.encode("logs/video_mem_prints_log.txt"), str.encode(data + '\n'))
		if len(data) > 14:
			ws.send_data(data)
		return
	try:
		with buffer_mutex:
			if filename not in buffers:
				buffers[filename] = {}
				buffers[filename]["time"] = 0
				buffers[filename]["data"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n"

			buffers[filename]["data"] += logstring

			if buffers[filename]["time"] == 0:
				buffers[filename]["time"] = time.time()
			if len(buffers[filename]["data"]) >= buffer_size:
				flush_buffer(filename)
	except KeyboardInterrupt:
		log_manager_exit()


def flush_buffer(filename):
	if filename in buffers and buffers[filename]["data"]:
		directory = os.path.dirname(filename)
		if not os.path.exists(directory):
			os.makedirs(directory)
		with open(filename, 'a') as f:
			f.write(buffers[filename]["data"])
			buffers[filename]["data"] = ""


def flush_all_buffers():
	global buffer_mutex
	with buffer_mutex:
		for filename in buffers:
			flush_buffer(filename)

print_callback = print_callback_t(print_logs)


def time_thread(stop_cond):
	global buffers, buffer_mutex, stop_thread
	while True:
		with buffer_mutex:
			for filename in buffers:
				if len(buffers[filename]["data"]) == 0:
					continue
				if time.time() - buffers[filename]["time"] > 10:	# flush at least every 10 seconds
					flush_buffer(filename)
					buffers[filename]["time"] = time.time()
		counter = 100
		while counter > 0:
			counter -= 1
			if stop_cond():
				return
			time.sleep(0.1)
		if stop_thread:
			break


stop_thread = False
print_thread = None


def log_manager_init():
	global print_thread

	files = glob.glob('./logs/*')
	for f in files:
		os.remove(f)

	print_thread = threading.Thread(target=time_thread, args=(lambda: stop_thread,))
	print_thread.start()
	ws.start_server(8765)


def log_manager_exit():
	global stop_thread, print_thread
	flush_all_buffers()
	stop_thread = True
	try:
		print_thread.join()
		ws.stop_server()
	except RuntimeError:
		print("Log thread has been completed. ", end='')
