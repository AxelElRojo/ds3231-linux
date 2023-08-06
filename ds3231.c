#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/rtc.h>
#define DS3231_LAST_MESSAGE _IOR('A', 0xFF, u8)
enum DS3231_REG_RTC{
	DS3231_REG_SEC = 0x00,
	DS3231_REG_MIN = 0x01,
	DS3231_REG_HRS = 0x02,
	DS3231_REG_WDAY = 0x03,
	DS3231_REG_MDAY = 0x04,
	DS3231_REG_MON = 0x05,
	DS3231_REG_YEAR = 0x06
};
enum DS3231_REG_CTL{
	DS3231_REG_CONTROL = 0x0E,
	DS3231_REG_STATUS = 0x0F,
	DS3231_REG_AGEOFF = 0x10,
	DS3231_REG_TEMPMSB = 0x11,
	DS3231_REG_TEMPLSB = 0x12
};
enum DS3231_RTC_MSK{
	DS3231_MSK_SEC = 0x0F,
	DS3231_MSK_10SEC = 0x70,
	DS3231_MSK_MIN = 0x0F,
	DS3231_MSK_10MIN = 0x70,
	DS3231_MSK_HR = 0x0F,
	DS3231_MSK_10HR = 0x10,
	DS3231_MSK_20HR = 0x20,
	DS3231_MSK_DAY = 0x0F,
	DS3231_MSK_10DAY = 0x30,
	DS3231_MSK_MON = 0x0F,
	DS3231_MSK_10MON = 0x10,
	DS3231_MSK_CENTURY = 0x80,
	DS3231_MSK_YEAR = 0x0F,
	DS3231_MSK_10YEAR = 0xF0
};
enum DS3231_CTL_MSK{
	DS3231_MSK_EOSC = 0x80,
	DS3231_MSK_INTCN = 0x04,
	DS3231_MSK_A2IE = 0x02,
	DS3231_MSK_A1IE = 0x01,
	DS3231_MSK_OSF = 0x80,
	DS3231_MSK_HR_SELECT = 0x40
};
u8 last_message;
int calculate_wday(int year, int month, int day){
	int century, yearOfCentury, h;
	if(month < 3){
		month += 12;
		year--;
	}
	century = year / 100;
	yearOfCentury = year % 100;
	h = (day + ((13 * (month + 1)) / 5) + yearOfCentury + (yearOfCentury / 4) + (century / 4) - (2 * century)) % 7;
	return (h + 5) % 7;
}
int calculate_yday(int day, int mon, int year){
    static const int days[2][13] = {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    return days[is_leap_year(year)][mon] + day;
}
static int ds3231_read_time(struct device* dev, struct rtc_time* tm){
	struct i2c_client* client = to_i2c_client(dev);
	s32 ret;
	u8 reg;
			ret = i2c_smbus_read_byte_data(client, DS3231_REG_SEC);
	if(ret < 0){
		pr_err("Error %d during seconds read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
	tm->tm_sec = 10 * (reg >> 4) + (reg & DS3231_MSK_SEC);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_MIN);
	if(ret < 0){
		pr_err("Error %d during minutes read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
	tm->tm_min = 10 * (reg >> 4) + (reg & DS3231_MSK_MIN);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_HRS);
	if(ret < 0){
		pr_err("Error %d during hours read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
	tm->tm_hour = 20 * ((reg >> 5) & 1) + 10 * ((reg >> 4) & 1) + (reg & DS3231_MSK_HR);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_MDAY);
	if(ret < 0){
		pr_err("Error %d during day read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
	tm->tm_mday = 10 * (reg >> 4) + (reg & DS3231_MSK_DAY);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_MON);
	if(ret < 0){
		pr_err("Error %d during month read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
	last_message = reg;
	tm->tm_mon = 10 * ((reg & DS3231_MSK_10MON) >> 4) + (reg & DS3231_MSK_MON);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_YEAR);
	if(ret < 0){
		pr_err("Error %d during year read\n", ret);
		return ret;
	}
	reg = (u8)(ret);
			tm->tm_year = 100 * (last_message >> 7) + 10 * (reg >> 4) + (reg & DS3231_MSK_YEAR);
	tm->tm_wday = calculate_wday(tm->tm_year, tm->tm_mon, tm->tm_mday);
	tm->tm_yday = calculate_yday(tm->tm_year, tm->tm_mon, tm->tm_mday);
	tm->tm_isdst = 0;
	last_message = reg;
	return 0;
}
static int ds3231_write_time(struct device *dev, struct rtc_time *tm){
	struct i2c_client* client = to_i2c_client(dev);
	s32 ret;
		ret = i2c_smbus_write_byte_data(client, DS3231_REG_SEC, ((tm->tm_sec / 10) << 4) | (tm->tm_sec % 10));
	if(ret < 0){
		pr_err("Error %d during seconds write\n", ret);
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, DS3231_REG_MIN, ((tm->tm_min / 10) << 4) | (tm->tm_min % 10));
	if(ret < 0){
		pr_err("Error %d during minutes write\n", ret);
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, DS3231_REG_HRS, ((tm->tm_hour / 20) << 5) | (((tm->tm_hour % 20) / 10) << 4) | ((tm->tm_hour % 20) % 10));
	if(ret < 0){
		pr_err("Error %d during hour write\n", ret);
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, DS3231_REG_MDAY, ((tm->tm_mday / 10) << 4) | (tm->tm_mday % 10));
	if(ret < 0){
		pr_err("Error %d during day write\n", ret);
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, DS3231_REG_MON, ((tm->tm_year >= 100) << 7) | ((tm->tm_mon / 10) << 4) | ((tm->tm_mon % 10)));
	if(ret < 0){
		pr_err("Error %d during month write\n", ret);
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, DS3231_REG_YEAR, (((tm->tm_year % 100) / 10) << 4) | ((tm->tm_year % 100) % 10));
	if(ret < 0){
		pr_err("Error %d during week write\n", ret);
		return ret;
	}
	last_message = (u8)ret;
	return 0;
}
static int ds3231_ioctl(struct device *dev, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case DS3231_LAST_MESSAGE:
			if(copy_to_user((void __user *)arg, &last_message, sizeof(u8)))
				return -EFAULT;
			return sizeof(u8);
		break;
		default:
			return -EINVAL;
	}
}
static const struct rtc_class_ops ds3231_rtc_ops = {
	.read_time = ds3231_read_time,
	.set_time = ds3231_write_time,
	.ioctl = ds3231_ioctl
};
static int ds3231_probe(struct i2c_client* client, const struct i2c_device_id* id){
	s32 ret;
	u8 reg;
	struct rtc_device *rtc;
	last_message = 0;
	rtc = devm_rtc_allocate_device(&client->dev);
	if(IS_ERR(rtc)){
		dev_err(&client->dev, "Failed to allocate RTC device\n");
		return PTR_ERR(rtc);
	}
	rtc->ops = &ds3231_rtc_ops;
	rtc->range_max = U32_MAX;
	i2c_set_clientdata(client, rtc);
	devm_rtc_device_register(&client->dev, "ds3231", &ds3231_rtc_ops, THIS_MODULE);
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_CONTROL);
	if(ret < 0){
		pr_err("Communication failed\n");
		return ret;
	}
	reg = (u8)ret;
			if(reg & DS3231_MSK_A1IE || reg & DS3231_MSK_A2IE || reg & DS3231_MSK_INTCN || reg & DS3231_MSK_EOSC){
		reg &= ~DS3231_MSK_A1IE;
		reg &= ~DS3231_MSK_A2IE;
		reg &= ~DS3231_MSK_INTCN;
		reg &= ~DS3231_MSK_EOSC;
		if(i2c_smbus_write_byte_data(client, DS3231_REG_CONTROL, reg) < 0) {
			pr_err("Communication failed\n");
			return ret;
		}
	}
	ret = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
	if(ret < 0){
		pr_err("Status read failed\n");
		return ret;
	}
	reg = (u8)ret;
		if(reg & DS3231_MSK_OSF){
		reg &= ~DS3231_MSK_OSF;
		if(i2c_smbus_write_byte_data(client, DS3231_REG_STATUS, reg) < 0){
			pr_err("Communication failed\n");
			return ret;
		}
	}
		ret = i2c_smbus_read_byte_data(client, DS3231_REG_HRS);
	if(ret < 0){
		pr_err("Hour mode read failed\n");
		return ret;
	}
	reg = (u8)ret;
	if(reg & DS3231_MSK_HR_SELECT){
		reg &= ~DS3231_MSK_HR_SELECT;
		if(i2c_smbus_write_byte_data(client, DS3231_REG_HRS, reg) < 0){
			pr_err("Communication failed\n");
			return ret;
		}
	}
	return 0;
}
static int ds3231_remove(struct i2c_client* client){
	return 0;
}
static const struct i2c_device_id ds3231_rtc_id[] = {
	{"ds3231", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ds3231_rtc_id);
static const struct of_device_id ds3231_of_match[] = {
	{.compatible = "maxim, ds3231"},
	{}
};
MODULE_DEVICE_TABLE(of, ds3231_of_match);
static struct i2c_driver ds3231_driver = {
	.driver = {
		.name = "ds3231",
		.of_match_table = ds3231_of_match
	},
	.probe = ds3231_probe,
	.remove = ds3231_remove,
	.id_table = ds3231_rtc_id,
};
module_i2c_driver(ds3231_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axel Morales Torres");
MODULE_DESCRIPTION("DS3231 driver");