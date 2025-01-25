//
// Created by aeinstein on 24.01.2025.
//

#include "xgo-proc.h"

static const struct proc_ops yaw_ops = {
	.proc_read = yaw_read,
};

static const struct proc_ops state_ops = {
	.proc_read = state_read,
};

static const struct proc_ops battery_ops = {
	.proc_read = battery_read,
};

static const struct proc_ops buttons_ops = {
	.proc_read = buttons_read,
};

static const struct proc_ops settings_ops = {
	.proc_read = settings_read,
    .proc_write = settings_write,
};

static const struct proc_ops translation_ops = {
    .proc_write = translation_write,
};

static ssize_t settings_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char buffer[5];
    size_t len;

    const char *setting_name = file->f_path.dentry->d_name.name;

    // Überprüfen, welcher Wert gelesen werden soll
    if (strcmp(setting_name, "verbose") == 0) {
      if(verbose) len = snprintf(buffer, sizeof(buffer), "1\n");
      else len = snprintf(buffer, sizeof(buffer), "0\n");

    } else if (strcmp(setting_name, "low_batt_level") == 0) {
        len = snprintf(buffer, sizeof(buffer), "%d\n", XGO_LOW_BATT);

    } else if (strcmp(setting_name, "sleep_ms_on_loop") == 0) {
    	len = snprintf(buffer, sizeof(buffer), "%d\n", XGO_MS_SLEEP_ON_LOOP);

    } else if (strcmp(setting_name, "shutdown_on_low_batt") == 0) {
      	if(XGO_SHUTDOWN_ON_LOW_BATT) len = snprintf(buffer, sizeof(buffer), "1\n");
      	else len = snprintf(buffer, sizeof(buffer), "0\n");

    } else if (strcmp(setting_name, "force_yaw") == 0) {
      	if(XGO_HOLD_YAW) len = snprintf(buffer, sizeof(buffer), "1\n");
      	else len = snprintf(buffer, sizeof(buffer), "0\n");

    } else {
        return -EINVAL; // Ungültiger Zugriff
    }

    return simple_read_from_buffer(user_buf, count, ppos, buffer, len);
}

static ssize_t translation_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos) {
  char buffer[6];
    const char *setting_name = file->f_path.dentry->d_name.name;

     // Eingabedaten überprüfen und kopieren
    if (count >= sizeof(buffer)) {
        return -EINVAL; // Eingabewert zu groß
    }

    if (copy_from_user(buffer, user_buf, count)) {
        return -EFAULT; // Fehler beim Kopieren
    }

    buffer[count] = '\0'; // Null-terminieren für Sicherheit


	int8_t speed = 0;
	snprintf(buffer, sizeof(buffer), "%d", speed);

    if (strcmp(setting_name, "speed_x") == 0) {
    	write_serial_data(XGO_VX, &speed, 1);

    } else if(strcmp(setting_name, "speed_z") == 0) {
    	write_serial_data(XGO_VYAW, &speed, 1);
    }

    return count;
}

static ssize_t settings_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos) {
    char buffer[6];
    const char *setting_name = file->f_path.dentry->d_name.name;

    // Eingabedaten überprüfen und kopieren
    if (count >= sizeof(buffer)) {
        return -EINVAL; // Eingabewert zu groß
    }

    if (copy_from_user(buffer, user_buf, count)) {
        return -EFAULT; // Fehler beim Kopieren
    }

    buffer[count] = '\0'; // Null-terminieren für Sicherheit

    // Einstellungen abhängig vom Namen ändern
    if (strcmp(setting_name, "verbose") == 0) {
        if(buffer[0] != 0x30) {
          verbose = true;
          pr_info("XGORider: verbose enabled");
        } else {
          verbose = false;
          pr_info("XGORider: verbose disabled");
        }

    } else if (strcmp(setting_name, "shutdown_on_low_batt") == 0) {
        if(buffer[0] != 0x30) XGO_SHUTDOWN_ON_LOW_BATT = true;
        else XGO_SHUTDOWN_ON_LOW_BATT = false;

    } else if (strcmp(setting_name, "force_yaw") == 0) {
        if(buffer[0] != 0x30) XGO_HOLD_YAW = true;
        else XGO_HOLD_YAW = false;

 	} else if (strcmp(setting_name, "low_batt_level") == 0) {
      	snprintf(buffer, sizeof(buffer), "%d", XGO_LOW_BATT);

    } else if (strcmp(setting_name, "sleep_ms_on_loop") == 0) {
      	snprintf(buffer, sizeof(buffer), "%d", XGO_MS_SLEEP_ON_LOOP);

    } else {
        return -EINVAL; // Ungültiger Zugriff
    }

    if(verbose) pr_info("Setting '%s' wurde auf '%s' aktualisiert\n", setting_name, buffer);

    return count;
}



static ssize_t buttons_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
	char decimal_buffer[3];
	snprintf(decimal_buffer, sizeof(decimal_buffer), "%d", XGO_Buttons);
	return simple_read_from_buffer(user_buf, count, ppos, decimal_buffer, strlen(decimal_buffer));
}

static ssize_t yaw_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char decimal_buffer[7];
    snprintf(decimal_buffer, sizeof(decimal_buffer), "%d", current_yaw);
    return simple_read_from_buffer(user_buf, count, ppos, decimal_buffer, strlen(decimal_buffer));
}

static ssize_t battery_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
	char decimal_buffer[4];
	snprintf(decimal_buffer, sizeof(decimal_buffer), "%d", battery);
    return simple_read_from_buffer(user_buf, count, ppos, decimal_buffer, strlen(decimal_buffer));
}

static ssize_t state_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char decimal_buffer[4];
    snprintf(decimal_buffer, sizeof(decimal_buffer), "%d", operational);
    return simple_read_from_buffer(user_buf, count, ppos, decimal_buffer, strlen(decimal_buffer));
}

static ssize_t leds_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos) {
  	pr_info("led write %s", user_buf);
    char buffer[8];  // Platz für "#RRGGBB" + Null-Terminierung
    uint8_t red, green, blue;

    // Prüfen, ob die Eingabegröße korrekt ist
    if (count < 7 || count > 8) {  // Erwartet "#RRGGBB" oder "#RRGGBB\n"
        return -EINVAL;           // Ungültige Eingabe
    }

    // Kopiere Daten aus dem Userspace in den Kernel-Puffer
    if (copy_from_user(buffer, user_buf, count)) {
        return -EFAULT;           // Fehler beim Kopieren der Daten
    }

    // Null-terminiere den String für Sicherheit
    buffer[count] = '\0';

    // Überprüfen, ob der String das erwartete Format enthält (beginnend mit '#' und gültigen hexadezimalen Zeichen)
    if (buffer[0] != '#' ||
        !isxdigit(buffer[1]) || !isxdigit(buffer[2]) ||
        !isxdigit(buffer[3]) || !isxdigit(buffer[4]) ||
        !isxdigit(buffer[5]) || !isxdigit(buffer[6])) {
        return -EINVAL;           // Ungültiges Format
    }

    // Konvertiere den Hex-String in RGB-Werte
    if (sscanf(buffer + 1, "%2hhx%2hhx%2hhx", &red, &green, &blue) != 3) {
        return -EINVAL;           // Fehler bei der Konvertierung
    }

    // Debug-Log zur Überprüfung der extrahierten RGB-Daten
    if(verbose) pr_info("LED %s - RGB Werte: R=%u, G=%u, B=%u\n", file->f_path.dentry->d_name.name, red, green, blue);

    // Hier können Sie die RGB-Werte auf die spezifische LED-Ansteuerung anwenden.
    // Beispiel: LEDs aktualisieren (in Abhängigkeit vom Dateinamen)
    if (strcmp(file->f_path.dentry->d_name.name, "0") == 0) {
        leds[0][0] = red;   // Setze den LED 1 Rotwert
        leds[0][1] = green;
        leds[0][2] = blue;
        write_serial_data(XGO_LED1, leds[0], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "1") == 0) {
        leds[1][0] = red;
        leds[1][1] = green;
        leds[1][2] = blue;
        write_serial_data(XGO_LED2, leds[1], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "2") == 0) {
        leds[2][0] = red;
        leds[2][1] = green;
        leds[2][2] = blue;
        write_serial_data(XGO_LED3, leds[2], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "3") == 0) {
        leds[3][0] = red;
        leds[3][1] = green;
        leds[3][2] = blue;
        write_serial_data(XGO_LED4, leds[3], 3);

    } else {
        return -EINVAL;  // Dateiname entspricht keiner bekannten LED
    }


    // Gib die Anzahl der verarbeiteten Bytes zurück
    return count;
}

static ssize_t action_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos){
	char buffer[4];
    uint8_t action;

    // Kopiere Daten aus dem Userspace in den Kernel-Puffer
    if (copy_from_user(buffer, user_buf, count)) {
        return -EFAULT;           // Fehler beim Kopieren der Daten
    }

    if (sscanf(buffer, "%hhu", &action) != 1) {
        return -EINVAL;           // Fehler bei der Konvertierung
    }

    pr_info("Action: %d", action);
	write_serial_data(XGO_ACTION, &action, 1);

  return count;
}


static const struct proc_ops led_ops = {
	.proc_write = leds_write,
};

static const struct proc_ops action_ops = {
	.proc_write = action_write,
};

static int createFilesystem(){
	proc_imu = proc_mkdir(PROC_DIR, NULL);

	if (!proc_imu) {
		destroyFilesystem();
		return -ENOMEM;
	}

    // /settings
    proc_settings = proc_mkdir("settings", proc_imu);
    proc_create("verbose", 0666, proc_settings, &settings_ops);
    proc_create("low_batt_level", 0666, proc_settings, &settings_ops);
    proc_create("shutdown_on_low_batt", 0666, proc_settings, &settings_ops);
    proc_create("force_yaw", 0666, proc_settings, &settings_ops);
    proc_create("sleep_ms_on_loop", 0666, proc_settings, &settings_ops);

	proc_yaw = proc_create("yaw", 0444, proc_imu, &yaw_ops);
	proc_state = proc_create("state", 0444, proc_imu, &state_ops);
	proc_buttons = proc_create("buttons", 0444, proc_imu, &buttons_ops);
	proc_battery = proc_create("battery", 0444, proc_imu, &battery_ops);
	proc_action = proc_create("action", 0666, proc_imu, &action_ops);
    proc_speed_x = proc_create("speed_x", 0666, proc_imu, &translation_ops);
    proc_speed_z = proc_create("speed_z", 0666, proc_imu, &translation_ops);

	if (!proc_yaw || !proc_state || !proc_battery || !proc_buttons) return -ENOMEM;


    // /leds
    proc_leds = proc_mkdir("leds", proc_imu);
	proc_led1 = proc_create("0", 0666, proc_leds, &led_ops);
	proc_led2 = proc_create("1", 0666, proc_leds, &led_ops);
	proc_led3 = proc_create("2", 0666, proc_leds, &led_ops);
	proc_led4 = proc_create("3", 0666, proc_leds, &led_ops);

	pr_info("XGORider: proc bindings created\n");
	return 0;
}

static void destroyFilesystem(){
	remove_proc_entry("yaw", proc_imu);
	remove_proc_entry("state", proc_imu);
	remove_proc_entry("buttons", proc_imu);
	remove_proc_entry("battery", proc_imu);
	remove_proc_entry("action", proc_imu);
    remove_proc_entry("speed_x", proc_imu);
	remove_proc_entry("speed_z", proc_imu);

	remove_proc_entry("0", proc_leds);
	remove_proc_entry("1", proc_leds);
	remove_proc_entry("2", proc_leds);
	remove_proc_entry("3", proc_leds);
	remove_proc_entry("leds", proc_imu);

    remove_proc_entry("verbose", proc_settings);
    remove_proc_entry("low_batt_level", proc_settings);
    remove_proc_entry("shutdown_on_low_batt", proc_settings);
    remove_proc_entry("force_yaw", proc_settings);
    remove_proc_entry("sleep_ms_on_loop", proc_settings);
    remove_proc_entry("settings", proc_imu);

	remove_proc_entry(PROC_DIR, NULL);
    pr_info("XGORider: proc bindings deleted\n");
}