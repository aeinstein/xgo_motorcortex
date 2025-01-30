//
// Created by aeinstein on 24.01.2025.
//

#include "xgo-serial.h"


union B2I16 conv;

struct message {
    char *data;
    size_t len;
    struct list_head list;
};

static LIST_HEAD(send_queue);     // Nachrichtenschlange
static DEFINE_MUTEX(queue_lock);  // Sperre zur Synchronisation


// Funktion zum Öffnen der seriellen Schnittstelle
static int open_serial_port(void) {
    pr_info("XGORider: open %s\n", SERIAL_PORT);
    serial_file = filp_open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC, 0);

    if (IS_ERR(serial_file)) {
        pr_err("XGORider: Failed to open serial device: %ld\n", PTR_ERR(serial_file));
        return PTR_ERR(serial_file);
    }

    pr_info("Serial device opened successfully\n");

    reader_thread = kthread_run(read_loop, NULL, "my_serial_thread");
    if(IS_ERR(reader_thread)) {
        pr_err("XGORider: Fehler beim Starten des Kernel-Threads\n");
        return PTR_ERR(reader_thread);
    }

    return 0;
}

static bool read_addr(const int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    read_serial_data(addr, read_buf, sizeof(read_buf));

    return true;
}

static int read_serial_data(size_t addr, char *buffer, size_t len) {
    const int mode = 0x02;
    size_t sum_data = (0x09 + mode + addr + len) % 256;
    sum_data = 255 - sum_data;

    loff_t pos = 0;

    unsigned char cmd[] = {0x55, 0x00, 0x09, mode, addr, len, sum_data, 0x00, 0xAA};

    if(verbose & VERBOSE_SERIAL){
    	pr_info( "XGORider: read len: %d\n", sizeof(cmd));

        pr_info( "XGORider: tx_data: ");
    	for (int i = 0; i < sizeof(cmd); i++) {
        	pr_cont("0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
    	}

    	pr_info( "\n");
	}

    addToSendQueue(cmd, sizeof(cmd));
    if(verbose & VERBOSE_SERIAL) pr_info( "XGORider: written %ld bytes\n", sizeof(cmd));

    msleep(200);
    return 0;
}

static int write_serial_data(const size_t addr, char * buffer, const size_t len){
    if(verbose & VERBOSE_SERIAL) pr_info( "XGORider: send %d bytes\n", len);

    loff_t pos = 0;

    int value_sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        value_sum += buffer[i];
    }

    if(verbose & VERBOSE_SERIAL) pr_info( "XGORider: val_sum %d\n", value_sum);

    const int mode = 0x01;
    int sum_data = ((len + 0x08) + mode + addr + value_sum) % 256;
    sum_data = 255 - sum_data;

    unsigned char cmd[len + 0x08];

    cmd[0] = 0x55;
    cmd[1] = 0x00;
    cmd[2] = len + 0x08;
    cmd[3] = mode;
    cmd[4] = addr;

    for (uint8_t i = 0; i < len; i++) {
        cmd[i + 0x05] = buffer[i];
    }

    cmd[len + 0x05] = sum_data;
    cmd[len + 0x06] = 0x00;
    cmd[len + 0x07] = 0xAA;

    if(verbose & VERBOSE_SERIAL) {
        pr_info( "XGORider: len: %lu\n", sizeof(cmd));

        pr_info( "XGORider: tx_data: ");

        for (int i = 0; i < sizeof(cmd); i++) {
            pr_cont("0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        pr_info( "\n");
    }

    addToSendQueue(cmd, sizeof(cmd));

    //kernel_write(serial_file, cmd, sizeof(cmd), &pos);

    return 0;
}


static int read_loop(void *data) {
    while (!kthread_should_stop()) {
        if (process_data()) {
            msleep(100);
        }

        msleep(100);
    }

    return 0;
}

static void stop_read_loop(void) {
    if(reader_thread) {
        kthread_stop(reader_thread);
    }
}


static bool process_data(void) {
    char buffer[32];

    size_t msg_index = 0;
    uint8_t rx_CHECK = 0;

    while(1){
        size_t num_bytes = kernel_read(serial_file, buffer, 1, 0);

        for (size_t i = 0; i < num_bytes; i++) {
            uint8_t num = buffer[i];
            rx_msg[msg_index++] = num;

            switch(rx_FLAG){
            case 0:
                if(num == 0x55) rx_FLAG++;
                break;

            case 1:
                if(num == 0x00) rx_FLAG++;
                break;

            case 2:
                rx_LEN = num;
                rx_FLAG++;
                break;

            case 3:
                rx_TYPE = num;
                rx_FLAG++;
                break;

            case 4:
                rx_ADDR = num;
                rx_COUNT = 0;
                rx_FLAG++;
                break;

            case 5:
                if(rx_COUNT == (rx_LEN - 9)) {
                    rx_data[rx_COUNT] = num;
                    rx_FLAG++;

                } else if (rx_COUNT < rx_LEN - 9) {
                    rx_data[rx_COUNT++] = num;
                }
                break;

            case 6:
                rx_CHECK = 0;

                for (size_t j = 0; j < rx_LEN - 8; j++) {
                    uint8_t t = rx_data[j];
                    rx_CHECK = rx_CHECK +t;
                }

                rx_CHECK = 255 - ((rx_LEN + rx_TYPE + rx_ADDR + rx_CHECK) % 256);

                if (num == rx_CHECK) {
                    if(verbose & VERBOSE_SERIAL) pr_info( "XGORider: checksum correct\n");
                    rx_FLAG++;
                } else {
                    pr_warn("XGORider: wrong checksum\n");
                    rx_FLAG = 0;
                    rx_COUNT = 0;
                    rx_ADDR = 0;
                    rx_LEN = 0;
                    return false;
                }
                break;

            case 7:
                if(num == 0x00){
                    rx_FLAG++;
                } else {
                    pr_warn("XGORider: no finish\n");
                    rx_FLAG = 0;
                    rx_COUNT = 0;
                    rx_ADDR = 0;
                    rx_LEN = 0;
                }
                break;

            case 8:
                if (num == 0xAA) {
                    rx_FLAG = 0;

                    if(verbose & VERBOSE_SERIAL) {
                        pr_info( "XGORider: rx_data: ");
                        for (size_t j = 0; j < msg_index; j++) {
                            pr_cont("0x%02X ", rx_msg[j]);
                        }
                        pr_info( "\n");

                        pr_info( "XGORider: rxlen: %d\n", rx_LEN);
                    }

                    setBackgroundValues();

                    return true;
                }

                pr_warn("XGORider: no finish\n");
                rx_FLAG = 0;
                rx_COUNT = 0;
                rx_ADDR = 0;
                rx_LEN = 0;
                break;

            default:
                return false;
            }
        }
    }
}

static void setBackgroundValues(void) {
    switch (rx_ADDR) {
        case XGO_YAW_INT:
            conv.b[0] = rx_data[1];
            conv.b[1] = rx_data[0];
            current_yaw = conv.i;
            if (verbose & VERBOSE_SERIAL) pr_info("XGORider: setting yaw to %d", current_yaw);
            break;

        case XGO_BATTERY:
            battery = rx_data[0];
            if (verbose & VERBOSE_SERIAL) pr_info("XGORider: setting battery to %d", battery);
            break;

        case XGO_STATE:
            operational = rx_data[0];
            break;

        default:
            break;
    }
}

// Funktion: Nachricht zur Sende-Warteschlange hinzufügen
static bool addToSendQueue(const char *cmd, size_t len) {
    struct message *new_msg;

    // Speicher für neue Nachricht reservieren
    new_msg = kmalloc(sizeof(*new_msg), GFP_KERNEL);
    if (!new_msg) {
        pr_err("XGORider: Speicherzuweisung für Nachricht fehlgeschlagen\n");
        return false;
    }

    new_msg->data = kmalloc(len, GFP_KERNEL);
    if (!new_msg->data) {
        pr_err("XGORider: Speicherzuweisung für Nachrichtendaten fehlgeschlagen\n");
        kfree(new_msg);
        return false;
    }

    memcpy(new_msg->data, cmd, len);
    new_msg->len = len;

    // Nachricht zur Warteschlange hinzufügen
    mutex_lock(&queue_lock);
    list_add_tail(&new_msg->list, &send_queue);
    mutex_unlock(&queue_lock);

    if (verbose & VERBOSE_SERIAL) pr_info("XGORider: Nachricht zur Warteschlange hinzugefügt (Länge: %zu)\n", len);
    doQueue();
    return true;
}

// Funktion: Nachrichten aus der Sende-Warteschlange verarbeiten
static void doQueue(void) {
    struct message *msg, *tmp;
    loff_t pos = 0;

    mutex_lock(&queue_lock);

    // Alle Nachrichten in der Warteschlange durchlaufen
    list_for_each_entry_safe(msg, tmp, &send_queue, list) {
        // Nachricht senden
        kernel_write(serial_file, msg->data, msg->len, &pos);

        if (verbose & VERBOSE_SERIAL) pr_info("XGORider: Nachricht gesendet (Länge: %zu)\n", msg->len);

        // Nachricht aus der Warteschlange entfernen und Speicher freigeben
        list_del(&msg->list);
        kfree(msg->data);
        kfree(msg);
    }

    mutex_unlock(&queue_lock);
}
