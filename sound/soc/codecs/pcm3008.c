/*
 * ALSA Soc PCM3008 codec support
 *
 * Author:	Hugo Villeneuve
 * Copyright (C) 2008 Lyrtech inc
 *
 * Based on AC97 Soc codec, original copyright follow:
 * Copyright 2005 Wolfson Microelectronics PLC.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Generic PCM3008 support.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "pcm3008.h"

#define PCM3008_VERSION "0.2"

#define PCM3008_RATES (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |	\
		       SNDRV_PCM_RATE_48000)

static struct snd_soc_dai_driver pcm3008_dai = {
	.name = "pcm3008-hifi",
	.playback = {
		.stream_name = "PCM3008 Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = PCM3008_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.stream_name = "PCM3008 Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = PCM3008_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
};

static void pcm3008_gpio_free(struct pcm3008_setup_data *setup)
{
	gpio_free(setup->dem0_pin);
	gpio_free(setup->dem1_pin);
	gpio_free(setup->pdad_pin);
	gpio_free(setup->pdda_pin);
}

static int pcm3008_soc_probe(struct snd_soc_codec *codec)
{
	struct pcm3008_setup_data *setup = codec->dev->platform_data;
	int ret = 0;

	printk(KERN_INFO "PCM3008 SoC Audio Codec %s\n", PCM3008_VERSION);


	
	ret = gpio_request(setup->dem0_pin, "codec_dem0");
	if (ret == 0)
		ret = gpio_direction_output(setup->dem0_pin, 1);
	if (ret != 0)
		goto gpio_err;

	
	ret = gpio_request(setup->dem1_pin, "codec_dem1");
	if (ret == 0)
		ret = gpio_direction_output(setup->dem1_pin, 0);
	if (ret != 0)
		goto gpio_err;

	
	ret = gpio_request(setup->pdad_pin, "codec_pdad");
	if (ret == 0)
		ret = gpio_direction_output(setup->pdad_pin, 1);
	if (ret != 0)
		goto gpio_err;

	
	ret = gpio_request(setup->pdda_pin, "codec_pdda");
	if (ret == 0)
		ret = gpio_direction_output(setup->pdda_pin, 1);
	if (ret != 0)
		goto gpio_err;

	return ret;

gpio_err:
	pcm3008_gpio_free(setup);

	return ret;
}

static int pcm3008_soc_remove(struct snd_soc_codec *codec)
{
	struct pcm3008_setup_data *setup = codec->dev->platform_data;

	pcm3008_gpio_free(setup);
	return 0;
}

#ifdef CONFIG_PM
static int pcm3008_soc_suspend(struct snd_soc_codec *codec)
{
	struct pcm3008_setup_data *setup = codec->dev->platform_data;

	gpio_set_value(setup->pdad_pin, 0);
	gpio_set_value(setup->pdda_pin, 0);

	return 0;
}

static int pcm3008_soc_resume(struct snd_soc_codec *codec)
{
	struct pcm3008_setup_data *setup = codec->dev->platform_data;

	gpio_set_value(setup->pdad_pin, 1);
	gpio_set_value(setup->pdda_pin, 1);

	return 0;
}
#else
#define pcm3008_soc_suspend NULL
#define pcm3008_soc_resume NULL
#endif

static struct snd_soc_codec_driver soc_codec_dev_pcm3008 = {
	.probe = 	pcm3008_soc_probe,
	.remove = 	pcm3008_soc_remove,
	.suspend =	pcm3008_soc_suspend,
	.resume =	pcm3008_soc_resume,
};

static int __devinit pcm3008_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_pcm3008, &pcm3008_dai, 1);
}

static int __devexit pcm3008_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

MODULE_ALIAS("platform:pcm3008-codec");

static struct platform_driver pcm3008_codec_driver = {
	.probe		= pcm3008_codec_probe,
	.remove		= __devexit_p(pcm3008_codec_remove),
	.driver		= {
		.name	= "pcm3008-codec",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(pcm3008_codec_driver);

MODULE_DESCRIPTION("Soc PCM3008 driver");
MODULE_AUTHOR("Hugo Villeneuve");
MODULE_LICENSE("GPL");
