import socket
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

HOST = '127.0.0.1'
PORT = 6767
CONCURRENCY = 50
REQUESTS_PER_CMD = 10000

# 支持的命令
COMMANDS = [
    'PING', 'ECHO', 'SET', 'GET', 'DEL', 'EXISTS',
    'KEYS', 'FLUSH', 'SAVE', 'LOAD', 'INFO', 'CONFIG', 'SELECT', 'AUTH', 'QUIT'
]

def send_command(cmd, args=None):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    if args:
        line = f"{cmd} {' '.join(str(a) for a in args)}\r\n"
    else:
        line = f"{cmd}\r\n"
    s.sendall(line.encode())
    resp = s.recv(4096)
    s.close()
    return resp

def perf_test_command(cmd, n, concurrency):
    latencies = []
    start = time.time()
    def task(i):
        t0 = time.time()
        # 针对不同命令构造参数
        if cmd == 'PING':
            send_command('PING')
        elif cmd == 'ECHO':
            send_command('ECHO', [f'hello_{i}'])
        elif cmd == 'SET':
            send_command('SET', [i, f'value_{i}'])
        elif cmd == 'GET':
            send_command('GET', [i])
        elif cmd == 'DEL':
            send_command('DEL', [i])
        elif cmd == 'EXISTS':
            send_command('EXISTS', [i])
        elif cmd == 'KEYS':
            send_command('KEYS')
        elif cmd == 'FLUSH':
            send_command('FLUSH')
        elif cmd == 'SAVE':
            send_command('SAVE')
        elif cmd == 'LOAD':
            send_command('LOAD')
        elif cmd == 'INFO':
            send_command('INFO')
        elif cmd == 'CONFIG':
            send_command('CONFIG', ['GET'])
        elif cmd == 'SELECT':
            send_command('SELECT', [0])
        elif cmd == 'AUTH':
            send_command('AUTH', ['password'])
        elif cmd == 'QUIT':
            send_command('QUIT')
        t1 = time.time()
        latencies.append(t1 - t0)
    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = [executor.submit(task, i) for i in range(n)]
        for f in as_completed(futures):
            pass
    end = time.time()
    total_time = end - start
    avg_latency = sum(latencies) / len(latencies) if latencies else 0
    qps = n / total_time if total_time > 0 else 0
    return qps, avg_latency

def main():
    print(f"性能测试: 服务器 {HOST}:{PORT}, 并发{CONCURRENCY}, 每类命令{REQUESTS_PER_CMD}次")
    results = {}
    for cmd in COMMANDS:
        print(f"测试 {cmd} ...", end='', flush=True)
        qps, avg_latency = perf_test_command(cmd, REQUESTS_PER_CMD, CONCURRENCY)
        results[cmd] = (qps, avg_latency)
        print(f" QPS={qps:.2f}, 平均延迟={avg_latency*1000:.2f}ms")
    print("\n=== 总结 ===")
    for cmd, (qps, avg_latency) in results.items():
        print(f"{cmd:8s} QPS={qps:.2f}, 平均延迟={avg_latency*1000:.2f}ms")

if __name__ == '__main__':
    main() 