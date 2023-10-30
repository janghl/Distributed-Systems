import random
import sys
import os
from datetime import datetime, timedelta

machine = sys.argv[1]
N = 5 if len(sys.argv) < 3 else int(sys.argv[2])

LOG_LEVELS = ["INFO", "WARNING", "ERROR"]
if not os.path.exists("generated_logs"):
    os.makedirs("generated_logs")

def random_past_time(current, delta_days=365):
    """Generate a random datetime object between now and delta_days ago."""
    return current - timedelta(days=random.randint(0, delta_days), 
                               hours=random.randint(0, 23),
                               minutes=random.randint(0, 59),
                               seconds=random.randint(0, 59))

def random_log_message(machine_num):
    level = random.choice(LOG_LEVELS)
    if level == "INFO":
        return f"[INFO] - Machine {machine_num}: Operation successful."
    elif level == "WARNING":
        return f"[WARNING] - Machine {machine_num}: New connection established."
    else:  # ERROR
        return f"[ERROR] - Machine {machine_num}: Operation failed."

# Current time
end_time = datetime.now()
start_time = random_past_time(end_time)

for i in range(N):
    with open(f"generated_logs/{machine}.{i}.log", "w") as f:
        f.write(f"TEST_SERVER_EXIT_UPON_CONNECTION\n")
        f.write(f"TEST_SERVER_CRASH_UPON_CONNECTION\n")
        f.write(f"ID - {machine}.{i}.log\n")
        current_time = start_time
        while current_time < end_time:
            log_message = random_log_message(i)
            log_entry = f"{current_time.strftime('%Y-%m-%d %H:%M:%S')} - {log_message}\n"
            f.write(log_entry)
            current_time += timedelta(minutes=random.randint(1, 10))




