//
// Created by aeinstein on 24.01.2025.
//

#ifndef XGO_PROC_H
#define XGO_PROC_H

#include <linux/proc_fs.h>
#include "xgo-drv.h"

static struct proc_dir_entry *proc_imu, *proc_yaw, *proc_state, *proc_buttons, *proc_battery, *proc_action, *proc_led1, *proc_led2, *proc_led3, *proc_led4;

static ssize_t yaw_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
static ssize_t battery_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
static ssize_t state_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
static ssize_t buttons_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);

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
  	printk(KERN_INFO "led write %s", user_buf);
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
    if(verbose) printk(KERN_INFO "LED %s - RGB Werte: R=%u, G=%u, B=%u\n", file->f_path.dentry->d_name.name, red, green, blue);

    // Hier können Sie die RGB-Werte auf die spezifische LED-Ansteuerung anwenden.
    // Beispiel: LEDs aktualisieren (in Abhängigkeit vom Dateinamen)
    if (strcmp(file->f_path.dentry->d_name.name, "led1") == 0) {
        leds[0][0] = red;   // Setze den LED 1 Rotwert
        leds[0][1] = green;
        leds[0][2] = blue;
        write_serial_data(XGO_LED1, leds[0], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "led2") == 0) {
        leds[1][0] = red;
        leds[1][1] = green;
        leds[1][2] = blue;
        write_serial_data(XGO_LED2, leds[1], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "led3") == 0) {
        leds[2][0] = red;
        leds[2][1] = green;
        leds[2][2] = blue;
        write_serial_data(XGO_LED3, leds[2], 3);

    } else if (strcmp(file->f_path.dentry->d_name.name, "led4") == 0) {
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

    printk(KERN_INFO "Action: %d", action);
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

	proc_yaw = proc_create("yaw", 0444, proc_imu, &yaw_ops);
	proc_state = proc_create("state", 0444, proc_imu, &state_ops);
	proc_buttons = proc_create("buttons", 0444, proc_imu, &buttons_ops);
	proc_battery = proc_create("battery", 0444, proc_imu, &battery_ops);
	
	if (!proc_yaw || !proc_state || !proc_battery || !proc_buttons) return -ENOMEM;
	
	proc_action = proc_create("action", 0666, proc_imu, &action_ops);
	proc_led1 = proc_create("led1", 0666, proc_imu, &led_ops);
	proc_led2 = proc_create("led2", 0666, proc_imu, &led_ops);
	proc_led3 = proc_create("led3", 0666, proc_imu, &led_ops);
	proc_led4 = proc_create("led4", 0666, proc_imu, &led_ops);

	pr_info("proc bindings created\n");
	return 0;
}

static void destroyFilesystem(){
	remove_proc_entry("yaw", proc_imu);
	remove_proc_entry("state", proc_imu);
	remove_proc_entry("buttons", proc_imu);
	remove_proc_entry("battery", proc_imu);
	
	remove_proc_entry("action", proc_imu);
	remove_proc_entry("led1", proc_imu);
	remove_proc_entry("led2", proc_imu);
	remove_proc_entry("led3", proc_imu);
	remove_proc_entry("led4", proc_imu);
	
	remove_proc_entry(PROC_DIR, NULL);
    pr_info("proc bindings deleted\n");
}

#endif //XGO_PROC_H
