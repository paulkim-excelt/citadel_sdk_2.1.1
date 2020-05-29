/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/
#include <board.h>
#include <errno.h>
#include <broadcom/i2c.h>
#include <string.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <sys_clock.h>

#define CFG_OFFSET                   0x00
#define CFG_RESET_SHIFT              31
#define CFG_EN_SHIFT                 30
#define CFG_M_RETRY_CNT_SHIFT        16
#define CFG_M_RETRY_CNT_MASK         0x0f

#define TIM_CFG_OFFSET               0x04
#define TIM_CFG_MODE_400_SHIFT       31

#define S_ADDR_OFFSET		0x08

#define M_FIFO_CTRL_OFFSET           0x0c
#define M_FIFO_RX_FLUSH_SHIFT        31
#define M_FIFO_TX_FLUSH_SHIFT        30
#define M_FIFO_RX_CNT_SHIFT          16
#define M_FIFO_RX_CNT_MASK           0x7f
#define M_FIFO_RX_THLD_SHIFT         8
#define M_FIFO_RX_THLD_MASK          0x3f

#define S_FIFO_CTRL_OFFSET	0x10
#define S_FIFO_RX_FLUSH_SHIFT	31
#define S_FIFO_TX_FLUSH_SHIFT	30

#define M_CMD_OFFSET                 0x30
#define M_CMD_START_BUSY_SHIFT       31
#define M_CMD_STATUS_SHIFT           25
#define M_CMD_STATUS_MASK            0x07
#define M_CMD_STATUS_SUCCESS         0x0
#define M_CMD_STATUS_LOST_ARB        0x1
#define M_CMD_STATUS_NACK_ADDR       0x2
#define M_CMD_STATUS_NACK_DATA       0x3
#define M_CMD_STATUS_TIMEOUT         0x4
#define M_CMD_STATUS_TLOW_MEXT_TX    0x5
#define M_CMD_STATUS_TLOW_MEXT_RX    0x6
#define M_CMD_PROTOCOL_SHIFT         9
#define M_CMD_PROTOCOL_MASK          0xf
#define M_CMD_PROTOCOL_BLK_WR        0x7
#define M_CMD_PROTOCOL_BLK_RD        0x8
#define M_CMD_PEC_SHIFT              8
#define M_CMD_RD_CNT_MASK            0xff

#define S_CMD_OFFSET		0x34
#define S_CMD_START_BUSY_SHIFT	31
#define S_CMD_STATUS_SHIFT	23

#define IE_OFFSET                    0x38
#define IE_M_RX_FIFO_FULL_SHIFT      31
#define IE_M_RX_THLD_SHIFT           30
#define IE_M_START_BUSY_SHIFT        28
#define IE_M_TX_UNDERRUN_SHIFT       27
#define IE_S_RX_FIFO_FULL_SHIFT      26
#define IE_S_RX_THLD_SHIFT           25
#define IE_S_RX_EVENT_SHIFT	24
#define IE_S_START_BUSY_SHIFT        23
#define IE_S_TX_UNDERRUN_SHIFT       22
#define IE_S_RD_EN_SHIFT	21

#define IS_OFFSET                    0x3c
#define IS_M_RX_FIFO_FULL_SHIFT      31
#define IS_M_RX_THLD_SHIFT           30
#define IS_M_START_BUSY_SHIFT        28
#define IS_M_TX_UNDERRUN_SHIFT       27
#define IS_S_RX_FIFO_FULL_SHIFT      26
#define IS_S_RX_THLD_SHIFT           25
#define IS_S_RX_EVENT_SHIFT	24
#define IS_S_START_BUSY_SHIFT        23
#define IS_S_TX_UNDERRUN_SHIFT       22
#define IS_S_RD_EN_SHIFT	21

#define M_TX_OFFSET                  0x40
#define M_TX_WR_STATUS_SHIFT         31
#define M_TX_DATA_MASK               0xff

#define M_RX_OFFSET                  0x44
#define M_RX_STATUS_SHIFT            30
#define M_RX_STATUS_MASK             0x03
#define M_RX_PEC_ERR_SHIFT           29
#define M_RX_DATA_MASK               0xff

#define S_TX_OFFSET		0x48
#define S_TX_WR_STATUS_SHIFT	31

#define S_RX_OFFSET		0x4c
#define S_RX_STATUS_SHIFT	30

#define I2C_TIMEOUT_MSEC             100
#define TX_RX_FIFO_SIZE            64

#define NUM_SLAVE_ADDR		4
#define MAX_SLAVE_NUM_WR_BUFS	8

struct i2c_cfg {
	struct i2c_handlers *handlers;
	void *handler_data;
};

/* I2C Master Support */

static int i2c_bcm58202_configure(struct device *dev, u32_t dev_config)
{
	union dev_config config = { .raw = dev_config };
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val;

	if (config.bits.is_master_device) {
		val = BIT(IE_M_TX_UNDERRUN_SHIFT);
		val |= BIT(IE_M_RX_THLD_SHIFT);
		val |= BIT(IE_M_RX_FIFO_FULL_SHIFT);

		sys_write32(val, base + IE_OFFSET);
	} else {
		val = BIT(IE_S_RD_EN_SHIFT);
		val |= BIT(IE_S_TX_UNDERRUN_SHIFT);
		val |= BIT(IE_S_RX_EVENT_SHIFT);
		val |= BIT(IE_S_RX_THLD_SHIFT);
		val |= BIT(IE_S_RX_FIFO_FULL_SHIFT);

		sys_write32(val, base + IE_OFFSET);
	}

	/* FIXME - Per the docs, 10bit is supported
	 * But not sure what to do here
	 */
	if (config.bits.use_10_bit_addr)
		return -ENOTSUP;

	switch (config.bits.speed) {
	case I2C_SPEED_STANDARD:
		sys_clear_bit(base + TIM_CFG_OFFSET, TIM_CFG_MODE_400_SHIFT);
		break;
	case I2C_SPEED_FAST:
		sys_set_bit(base + TIM_CFG_OFFSET, TIM_CFG_MODE_400_SHIFT);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int i2c_bcm58202_busy_poll(struct device *dev)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	int to;

	for (to = I2C_TIMEOUT_MSEC * USEC_PER_MSEC; to > 0; to--) {
		if (!sys_test_bit(base + M_CMD_OFFSET, M_CMD_START_BUSY_SHIFT))
			return 0;

		k_busy_wait(1);
	}

	SYS_LOG_ERR("Timeout Waiting for I2C to be free");

	return -EAGAIN;
}

static int i2c_bcm58202_transfer_one(struct device *dev, struct i2c_msg *msgs,
				     u16_t addr)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val, i;
	int rc;

	rc = i2c_bcm58202_busy_poll(dev);
	if (rc)
		goto err;

	/* Set the address */
	val = addr << 1;
	val &= M_TX_DATA_MASK;
	if (msgs->flags & I2C_MSG_READ)
		val |= 1;
	sys_write32(val, base + M_TX_OFFSET);

	if (msgs->flags & I2C_MSG_READ) {
		/* Max read allowed in a block read is 255 bytes (limited by hw)
		 */
		if (msgs->len > M_CMD_RD_CNT_MASK) {
			SYS_LOG_ERR("I2C controller does nor support read "
				    "sizes larger than %d bytes [requested:%d]",
				    M_CMD_RD_CNT_MASK, msgs->len);
			return -EINVAL;
		}

		if (msgs->flags & I2C_MSG_STOP) {
			/* Now we can activate the transfer */
			val = BIT(M_CMD_START_BUSY_SHIFT);
			val |= M_CMD_PROTOCOL_BLK_RD << M_CMD_PROTOCOL_SHIFT;
			val |= msgs->len;
			sys_write32(val, base + M_CMD_OFFSET);
		}
	} else {
		/*
		 * For a write transaction, load data into the TX FIFO.
		 * Only allow loading up to TX FIFO size/2 bytes of
		 * data since there could be previous write transfers programmed
		 * already without the I2C_MSG_STOP bit Set
		 */
		u32_t offset, remaining, write_len, started = 0;

		offset = 0;
		remaining = msgs->len;
continue_tx:
		/* Split transactions to chunks for size TX_FIFO_SIZE/2 */
		write_len = min(TX_RX_FIFO_SIZE/2, remaining);
		for (i = 0; i < write_len - 1; i++)
			sys_write32(msgs->buf[offset + i], base + M_TX_OFFSET);

		/* mark the last byte */
		if ((msgs->flags & I2C_MSG_STOP) && (write_len == remaining))
			sys_write32(msgs->buf[offset + i] |
				    BIT(M_TX_WR_STATUS_SHIFT),
				    base + M_TX_OFFSET);
		else
			sys_write32(msgs->buf[offset + i], base + M_TX_OFFSET);

		/* Start the transfer only if we have more than FIFO size worth
		 * of bytes to send or if a stop condition is requested after
		 * the transfer
		 */
		if ((write_len < remaining) || (msgs->flags & I2C_MSG_STOP)) {
			if (!started) {
				/* Now we can activate the transfer */
				val = BIT(M_CMD_START_BUSY_SHIFT);
				val |= M_CMD_PROTOCOL_BLK_WR <<
				       M_CMD_PROTOCOL_SHIFT;
				sys_write32(val, base + M_CMD_OFFSET);
				started = 1;
			}
		}

		/* For all chunks except the last one */
		if (write_len < remaining) {
			/* For lengths larger than the fifo size, poll on the tx
			 * underrun bit to detect the fifo emptiness and queue
			 * more data
			 */
			while ((sys_read32(base + IS_OFFSET) &
				BIT(IE_M_TX_UNDERRUN_SHIFT)) == 0)
				k_busy_wait(1);
			/* Clear the tx underrun status bit */
			sys_write32(BIT(IS_M_TX_UNDERRUN_SHIFT),
				base + IS_OFFSET);

			offset += write_len;
			remaining -= write_len;
			if (remaining)
				goto continue_tx;
		}
	}

	if (msgs->flags & I2C_MSG_STOP) {
		/* For a read, start reading out of RX fifo immediately since
		 * the requested read size can be greater than the fifo size
		 */
		if (msgs->flags & I2C_MSG_READ) {
			for (i = 0; i < msgs->len; i++) {
				u32_t busy, rx_cnt, rx_full, timeout;

				/* Wait for some data to be received or
				 * the buffer to be full or transaction be done
				 */
				timeout = I2C_TIMEOUT_MSEC * USEC_PER_MSEC/10;
				do {
					busy = sys_read32(base + M_CMD_OFFSET) &
					       BIT(M_CMD_START_BUSY_SHIFT);

					rx_cnt = sys_read32(base +
							    M_FIFO_CTRL_OFFSET);
					rx_cnt >>= M_FIFO_RX_CNT_SHIFT;
					rx_cnt &= M_FIFO_RX_CNT_MASK;

					rx_full = sys_read32(base + IS_OFFSET) &
						  BIT(IS_M_RX_FIFO_FULL_SHIFT);
					/* Clear the fifo full bit */
					if (rx_full) {
						sys_write32(
						   BIT(IS_M_RX_FIFO_FULL_SHIFT),
						   base + IS_OFFSET);
					}
					if (--timeout == 0) {
						rc = -ETIMEDOUT;
						goto err;
					}
				} while ((busy) && (rx_cnt == 0) && (!rx_full));

				/* Read a byte */
				msgs->buf[i] = sys_read32(base + M_RX_OFFSET) &
					       M_RX_DATA_MASK;
			}
		}

		/* Wait for the Read/Write command to finish */
		rc = i2c_bcm58202_busy_poll(dev);
		if (rc)
			goto err;

		val = sys_read32(base + M_CMD_OFFSET);
		val >>= M_CMD_STATUS_SHIFT;
		val &= M_CMD_STATUS_MASK;

		switch (val) {
		case M_CMD_STATUS_SUCCESS:
			break;

		case M_CMD_STATUS_LOST_ARB:
			SYS_LOG_DBG("lost bus arbitration");
			rc = -EAGAIN;
			goto err;

		case M_CMD_STATUS_NACK_ADDR:
			SYS_LOG_DBG("NAK addr:0x%02x", addr);
			rc = -ENXIO;
			goto err;

		case M_CMD_STATUS_NACK_DATA:
			SYS_LOG_DBG("NAK data");
			rc = -ENXIO;
			goto err;

		case M_CMD_STATUS_TIMEOUT:
			SYS_LOG_DBG("bus timeout");
			rc = -ETIMEDOUT;
			goto err;
		case M_CMD_STATUS_TLOW_MEXT_TX:
			/* Expected status for transfer > tx fifo size */
			if (msgs->len > TX_RX_FIFO_SIZE)
				break;
		default:
			SYS_LOG_DBG("Unknown Error : %d", val);
			rc = -EIO;
			goto err;
		}
	}
	return 0;

err:
	/* flush both TX/RX FIFOs */
	val = BIT(M_FIFO_RX_FLUSH_SHIFT) | BIT(M_FIFO_TX_FLUSH_SHIFT);
	sys_write32(val, base + M_FIFO_CTRL_OFFSET);

	/* HW most likely locked up, re-init */
	dev->config->init(dev);

	return rc;
}

static int i2c_bcm58202_transfer_multi(struct device *dev, struct i2c_msg *msgs,
				       u8_t num_msgs, u16_t addr)
{
	int i, rc;
	struct i2c_msg *msgs_chk = msgs;

	if (!msgs_chk)
		return -EINVAL;


	for (i = 0; i < num_msgs; i++, msgs_chk++) {
		if (!msgs_chk->buf)
			return -EINVAL;
	}

	for (i = 0; i < num_msgs; i++, msgs++) {
		rc = i2c_bcm58202_transfer_one(dev, msgs, addr);
		if (rc)
			return rc;
	}

	return 0;
}

/* I2C Slave Support */

int i2c_slave_set_address(struct device *dev, u8_t id, u8_t address)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val, offset;

	if (address == 0) {
		SYS_LOG_DBG("Invalid address");
		return -EINVAL;
	}

	if (id >= NUM_SLAVE_ADDR) {
		SYS_LOG_ERR("Invalid id %d", id);
		return -EINVAL;
	}

	offset = id * 8;

	address = (address & 0x7f) | (0x1 << 7);
	val = sys_read32(base + S_ADDR_OFFSET);
	val &= ~(0xff << offset);
	val |= (address << offset);
	sys_write32(val, base + S_ADDR_OFFSET);

	return 0;
}

int i2c_slave_get_address(struct device *dev, u8_t id, u8_t *address)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val, offset;

	if (address == NULL) {
		SYS_LOG_ERR("address is NULL");
		return -EINVAL;
	}

	if (id >= NUM_SLAVE_ADDR) {
		SYS_LOG_ERR("Invalid id %d", id);
		return -EINVAL;
	}

	offset = id * 8;

	val = sys_read32(base + S_ADDR_OFFSET);
	val = val >> offset;
	*address = val & 0x7f;

	return 0;
}

static int i2c_slave_write_handler(struct device *dev, struct i2c_msg *msg)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t status, val, i = 0;
	int rc;

	/* FIXME - This is a little hacky here.  There is no way to tell
	 * how many buffers are in the message, and the spec doesn't
	 * seem to have an upper bound.  So, use the #define for the max amount
	 * and look at system log for the error below denoting the buf full.
	 */
	msg->buf = k_malloc(sizeof(u8_t) * MAX_SLAVE_NUM_WR_BUFS);
	if (!msg->buf)
		return -ENOMEM;

	do {
		/* Pull the msg info out of the FIFO */
		val = sys_read32(base + S_RX_OFFSET);
		status = val >> S_RX_STATUS_SHIFT;
		if (status == 0x0) {
			SYS_LOG_ERR("RX FIFO empty");

			/* HW most likely locked up, re-init */
			dev->config->init(dev);

			rc = -EIO;
			goto err;
		}

		if (i >= MAX_SLAVE_NUM_WR_BUFS) {
			SYS_LOG_ERR("Message buffer full.  Dropping message");

			rc = -EIO;
			goto err;
		}

		/* Save the payload part of the FIFO data as the i2c message */
		msg->buf[i] = val & 0xff;
		i++;
	} while (status != 0x3);

	msg->len = i;

	return 0;

err:
	k_free(msg->buf);
	msg->buf = NULL;
	msg->len = 0;
	return rc;
}

static void i2c_slave_wr(struct device *dev)
{
	struct i2c_cfg *cfg = dev->driver_data;
	struct i2c_msg *msg;
	int rc;

	/* Alloc the message container to pass to the callback handler */
	msg = k_malloc(sizeof(struct i2c_msg));
	if (!msg)
		return;

	msg->buf = NULL;

	/* Remove the data from the FIFO and add it to the message container */
	rc = i2c_slave_write_handler(dev, msg);
	if (rc)
		goto err;

	if (cfg->handlers->wr_handler)
		cfg->handlers->wr_handler(cfg->handler_data, msg);
	else
		/* If no callback throw the message away.  This allows for it to
		 * be pulled off the FIFO and a new message to be received
		 */
		goto err1;

	return;

err1:
	if (msg->buf)
		k_free(msg->buf);
err:
	k_free(msg);
}

void i2c_slave_poll(struct device *dev)
{
	struct i2c_cfg *cfg = dev->driver_data;
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val, curr_irqs;

	/* disable all interrupts */
	curr_irqs = sys_read32(base + IE_OFFSET);
	sys_write32(0, base + IE_OFFSET);

	/* determine what kind of event it is */
	val = sys_read32(base + IS_OFFSET);

	/* The read/write is from the perspective of the master.  So a write
	 * from the master is a receive of data in the FIFO on the slave, and
	 * a read request is a need by the slave driver to transmit the data
	 */
	if (val & 0x7 << IS_S_RX_EVENT_SHIFT)
		i2c_slave_wr(dev);

	/* If a read event, call the callback handler directly so that it can
	 * transmit the message to the i2c master
	 */
	if (val & 0x1 << IS_S_RD_EN_SHIFT &&
	    cfg->handlers->rd_handler)
		cfg->handlers->rd_handler(cfg->handler_data);

	/* Clear the interrupts */
	sys_write32(val, base + IS_OFFSET);

	/* Re-enable the interrupts */
	sys_write32(curr_irqs, base + IE_OFFSET);
}

void i2c_register_handlers(struct device *dev, struct i2c_handlers *handlers,
			   void *handler_data)
{
	struct i2c_cfg *cfg = dev->driver_data;

	cfg->handlers = handlers;
	cfg->handler_data = handler_data;
}

static int i2c_slave_bcm58202_busy_poll(struct device *dev)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	int to;

	for (to = I2C_TIMEOUT_MSEC * USEC_PER_MSEC; to > 0; to--) {
		if (!sys_test_bit(base + S_CMD_OFFSET, S_CMD_START_BUSY_SHIFT))
			return 0;

		k_busy_wait(1);
	}

	SYS_LOG_ERR("Timeout Waiting for I2C to be free");

	return -EAGAIN;
}

int i2c_slave_read_handler(struct device *dev, struct i2c_msg *msg)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u32_t val, i;
	int rc;

	if (msg == NULL)
		return -EINVAL;

	if (msg->len > TX_RX_FIFO_SIZE)
		return -EIO;

	for (i = 0; i < msg->len; i++) {
		val = msg->buf[i];
		if (i == (msg->len - 1))
			val |= BIT(S_TX_WR_STATUS_SHIFT);

		sys_write32(val, base + S_TX_OFFSET);
	}
	val = BIT(S_CMD_START_BUSY_SHIFT);
	sys_write32(val, base + S_CMD_OFFSET);

	rc = i2c_slave_bcm58202_busy_poll(dev);
	if (rc != 0) {
		SYS_LOG_ERR("i2c_slave_bcm58202_busy_poll failed");
		/* HW most likely locked up, re-init */
		dev->config->init(dev);
		return -EIO;
	}

	return rc;
}

static int i2c_bcm58202_init(struct device *dev)
{
	mem_addr_t base = (mem_addr_t) dev->config->config_info;
	u8_t slave_addr[NUM_SLAVE_ADDR];
	u32_t val;
	int i;

	for (i = 0; i < NUM_SLAVE_ADDR; i++)
		i2c_slave_get_address(dev, i, &slave_addr[i]);

	/* put controller in reset */
	val = sys_read32(base + CFG_OFFSET);
	val |= BIT(CFG_RESET_SHIFT);
	val &= ~BIT(CFG_EN_SHIFT);
	sys_write32(val, base + CFG_OFFSET);

	/* wait 100 usec per spec */
	k_busy_wait(100);

	/* bring controller out of reset */
	val &= ~BIT(CFG_RESET_SHIFT);
	sys_write32(val, base + CFG_OFFSET);

	/* Set the RX FIFO threshold to 1 */
	sys_write32(0x1 << M_FIFO_RX_THLD_SHIFT, base + M_FIFO_CTRL_OFFSET);

	/* flush TX/RX FIFOs and set RX FIFO threshold to zero */
	val = BIT(M_FIFO_RX_FLUSH_SHIFT) | BIT(M_FIFO_TX_FLUSH_SHIFT) |
	      BIT(S_FIFO_RX_FLUSH_SHIFT) | BIT(S_FIFO_TX_FLUSH_SHIFT);
	sys_write32(val, base + M_FIFO_CTRL_OFFSET);

	/* disable all interrupts */
	sys_write32(0, base + IE_OFFSET);

	/* clear all pending interrupts */
	sys_write32(~0, base + IS_OFFSET);

	for (i = 0; i < NUM_SLAVE_ADDR; i++)
		i2c_slave_set_address(dev, i, slave_addr[i]);

	/* Enable */
	sys_set_bit(base + CFG_OFFSET, CFG_EN_SHIFT);

	return 0;
}

struct i2c_driver_api i2c_bcm58202_api = {
	.configure = i2c_bcm58202_configure,
	.transfer = i2c_bcm58202_transfer_multi,
};

static struct i2c_cfg i2c0_cfg_data;

DEVICE_AND_API_INIT(bcm58202_i2c0, CONFIG_I2C0_NAME, i2c_bcm58202_init,
		    &i2c0_cfg_data, (void *)I2C0_BASE_ADDR, POST_KERNEL,
		    CONFIG_I2C_DRIVER_INIT_PRIORITY, &i2c_bcm58202_api);

static struct i2c_cfg i2c1_cfg_data;

DEVICE_AND_API_INIT(bcm58202_i2c1, CONFIG_I2C1_NAME, i2c_bcm58202_init,
		    &i2c1_cfg_data, (void *)I2C1_BASE_ADDR, POST_KERNEL,
		    CONFIG_I2C_DRIVER_INIT_PRIORITY, &i2c_bcm58202_api);

static struct i2c_cfg i2c2_cfg_data;

DEVICE_AND_API_INIT(bcm58202_i2c2, CONFIG_I2C2_NAME, i2c_bcm58202_init,
		    &i2c2_cfg_data, (void *)I2C2_BASE_ADDR, POST_KERNEL,
		    CONFIG_I2C_DRIVER_INIT_PRIORITY, &i2c_bcm58202_api);
