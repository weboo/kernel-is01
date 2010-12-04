/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <mach/qdsp5v2/qdsp5afecmdi.h>
#include <mach/qdsp5v2/qdsp5afemsg.h>
#include <mach/qdsp5v2/afe.h>
#include <mach/msm_adsp.h>

#define DEBUG
#ifdef DEBUG
#define dprintk(format, arg...) \
printk(KERN_DEBUG format, ## arg)
#else
#define dprintk(format, arg...) do {} while (0)
#endif

#define AFE_MAX_TIMEOUT 30 /* 30 ms */
#define AFE_MAX_CLNT 6 /* 6 HW path defined so far */

struct msm_afe_state {
	struct msm_adsp_module *mod;
	struct msm_adsp_ops    adsp_ops;
	struct mutex           lock;
	u8                     in_use;
	u8                     codec_config[AFE_MAX_CLNT];
	wait_queue_head_t      wait;
};

static struct msm_afe_state the_afe_state;

#define afe_send_queue(afe, cmd, len) \
  msm_adsp_write(afe->mod, QDSP_apuAfeQueue, \
	cmd, len)

static void afe_dsp_event(void *data, unsigned id, size_t len,
			    void (*getevent)(void *ptr, size_t len))
{
	struct msm_afe_state *afe = data;

	dprintk("%s: msg_id %d \n", __func__, id);

	switch (id) {
	case AFE_APU_MSG_CODEC_CONFIG_ACK: {
		struct afe_msg_codec_config_ack afe_ack;
		getevent(&afe_ack, AFE_APU_MSG_CODEC_CONFIG_ACK_LEN);
		dprintk("%s: device_id: %d device activity: %d\n", __func__,
		afe_ack.device_id, afe_ack.device_activity);
		if (afe_ack.device_activity == AFE_MSG_CODEC_CONFIG_DISABLED)
			afe->codec_config[afe_ack.device_id] = 0;
		else
			afe->codec_config[afe_ack.device_id] =
			afe_ack.device_activity;

		wake_up(&afe->wait);
		break;
	}
	default:
		pr_err("unexpected message from afe \n");
	}

	return;
}

static void afe_dsp_codec_config(struct msm_afe_state *afe,
	u8 path_id, u8 enable, struct msm_afe_config *config)
{
	struct afe_cmd_codec_config cmd;

	dprintk("%s() %p\n", __func__, config);
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd_id = AFE_CMD_CODEC_CONFIG_CMD;
	cmd.device_id = path_id;
	cmd.activity = enable;
	if (config) {
		dprintk("%s: sample_rate %x ch mode %x vol %x\n",
			__func__, config->sample_rate,
			config->channel_mode, config->volume);
		cmd.sample_rate = config->sample_rate;
		cmd.channel_mode = config->channel_mode;
		cmd.volume = config->volume;
	}
	afe_send_queue(afe, &cmd, sizeof(cmd));
}

int afe_enable(u8 path_id, struct msm_afe_config *config)
{
	struct msm_afe_state *afe = &the_afe_state;
	int rc;

	dprintk("%s: path %d\n", __func__, path_id);
	mutex_lock(&afe->lock);
	if (!afe->in_use) {
		/* enable afe */
		rc = msm_adsp_get("AFETASK", &afe->mod, &afe->adsp_ops, afe);
		if (rc < 0) {
			pr_err("%s: failed to get AFETASK module\n", __func__);
			goto error;
		}
		rc = msm_adsp_enable(afe->mod);
		if (rc < 0)
			goto error;
	}
	/* Issue codec config command */
	afe_dsp_codec_config(afe, path_id, 1, config);
	rc = wait_event_timeout(afe->wait,
		afe->codec_config[path_id],
		msecs_to_jiffies(AFE_MAX_TIMEOUT));
	if (!rc) {
		pr_err("AFE failed to respond within 30ms\n");
		rc = -ENODEV;
		goto error;
	} else
		rc = 0;
	afe->in_use++;
error:
	mutex_unlock(&afe->lock);
	return rc;
}
EXPORT_SYMBOL(afe_enable);

int afe_disable(u8 path_id)
{
	struct msm_afe_state *afe = &the_afe_state;
	int rc;

	mutex_lock(&afe->lock);

	BUG_ON(!afe->in_use);
	dprintk("%s() path_id:%d codec state:%d\n", __func__, path_id,
	afe->codec_config[path_id]);
	afe_dsp_codec_config(afe, path_id, 0, NULL);
	rc = wait_event_timeout(afe->wait,
		!afe->codec_config[path_id],
		msecs_to_jiffies(AFE_MAX_TIMEOUT));
	if (!rc) {
		pr_err("AFE failed to respond within 30ms\n");
		rc = -1;
		goto error;
	} else
		rc = 0;
	afe->in_use--;
	dprintk("%s() in_use:%d \n", __func__, afe->in_use);
	if (!afe->in_use) {
		msm_adsp_disable(afe->mod);
		msm_adsp_put(afe->mod);
	}
error:
	mutex_unlock(&afe->lock);
	return 0;
}
EXPORT_SYMBOL(afe_disable);

static int __init afe_init(void)
{
	struct msm_afe_state *afe = &the_afe_state;

	dprintk("AFE driver init\n");

	memset(afe, 0, sizeof(struct msm_afe_state));
	afe->adsp_ops.event = afe_dsp_event;
	mutex_init(&afe->lock);
	init_waitqueue_head(&afe->wait);

	return 0;
}

static void __exit afe_exit(void)
{
	dprintk("AFE driver exit\n");
	if (the_afe_state.mod)
		msm_adsp_put(the_afe_state.mod);
	return;
}

module_init(afe_init);
module_exit(afe_exit);

MODULE_DESCRIPTION("MSM AFE driver");
MODULE_LICENSE("Dual BSD/GPL v2");
