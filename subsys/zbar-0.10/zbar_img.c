#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <kernel.h>

#include <image.h>
#include <zbar.h>

static zbar_processor_t *processor = NULL;

//scan one y8 image frame
static int scan_one_y8_image (unsigned char *y8_img_buf, unsigned int y8_img_width, unsigned int y8_img_height, char *decoded_output, unsigned int *output_str_len)
{
	int rc = 0;
	int found = 0;
	zbar_image_t *dstimg =NULL; 
	unsigned char *img_buff = y8_img_buf;

#if 0
	unsigned char *img_buff = (unsigned char *)ZBAR_MALLOC(y8_img_width * y8_img_height);
	ZBAR_RETURN_ON_NULL(img_buff, -1);

	memcpy(img_buff, y8_img_buf, y8_img_width * y8_img_height);
#endif

	dstimg = zbar_image_create();
	if(!dstimg)
	{
		ZBAR_PRINT("Fail to create dst image\n");
		rc = -1;
		goto error_exit;
	}

	ZBAR_RETURN_ON_NULL(img_buff, -1);
	zbar_image_set_format(dstimg, *(unsigned long*)"Y800");
	zbar_image_set_size(dstimg, y8_img_width, y8_img_height);
	zbar_image_set_data(dstimg, img_buff, (y8_img_width * y8_img_height), zbar_image_free_data);
	dstimg->cleanup = zbar_image_free_data;
	dstimg->refcnt = 1;
	dstimg->src = NULL;
	dstimg->srcidx = -1;
	dstimg->seq = 0;

	rc = zbar_process_image(processor, dstimg); //_zbar_process_image();
	if(rc<0)
	{
		ZBAR_PRINT("Fail to process image\n");
		rc = -1;
		goto error_exit;
	}

	// output result data
	int count = 0;
	const zbar_symbol_t *sym = zbar_image_first_symbol(dstimg);
	for(; sym; sym = zbar_symbol_next(sym))
	{
		zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
		if(typ == ZBAR_PARTIAL)
			continue;
		else
		{
			char *pos = decoded_output + count;
			count += sprintf(pos, "%s%s:%s\n",
				zbar_get_symbol_name(typ),
				zbar_get_addon_name(typ),
				zbar_symbol_get_data(sym));

			found++;
		}
	}

	*output_str_len = count;

error_exit:
	if(dstimg->syms)
		_zbar_symbol_set_free(dstimg->syms);

	dstimg->data = NULL;  //To avoid configASSERT() triggered in zbar_image_free_data(), let caller to free even this is a mallocated buffer.
	zbar_image_destroy(dstimg);

	if(rc<0)
		return rc;

	if(!found)
	{
		ZBAR_PRINT("No symbol found\n");
		return (-1);
	}

    return(found);
}

/*
 * y8_img_buff: Y800 image array pointer
 * y8_img_len: Y800 image array len
 * decoded_output: decoded string from y8 image
*  output_count: output string len
 */
int zbarimg_scan_one_y8_image(unsigned char *y8_img_buff, unsigned int y8_img_width, unsigned int y8_img_height, char *decoded_output, unsigned int *output_count)
{
	int rc=0;

	processor = zbar_processor_create(0);
	if(!processor)
	{
		ZBAR_PRINT("Fail to create processor\n");
		return -1;
	}

	if(zbar_processor_init(processor, NULL, 0))
	{
		ZBAR_PRINT("Fail to init processor\n");
		rc=-1;
		goto error;
	}

	rc = scan_one_y8_image(y8_img_buff, y8_img_width, y8_img_height, decoded_output, output_count);
	if(rc<0)
	{
		ZBAR_PRINT("Fail to scan image, rc=%d\n", rc);
		rc=-1;
		goto error;
	}

error:
	zbar_processor_destroy(processor);
	return rc;
}


#if 1
#ifdef CONFIG_ZBAR_USE_STATIC_BUFFER
/* Re-use unicam capture buffer for YUV => Y800 conversion
 * This is a hack to overcome the memory crunch resulting from an unoptimized
 * binarize function.
 */
#ifdef CONFIG_BRCM_DRIVER_UNICAM
extern unsigned char capture_buffer[640*480];
#else
unsigned char capture_buffer[640*480];
#endif
unsigned char *grey_buff = &capture_buffer[0];
#endif
//same with convert_yuv_unpack(), convert yuy2 buff to grayscale, only Y vector reserved
static int zbar_convert_yuy2(zbar_image_t *dst,
                                const zbar_format_def_t *dstfmt,
                                const zbar_image_t *src,
                                const zbar_format_def_t *srcfmt)
{
    unsigned long dstn = dst->width * dst->height;
    dst->datalen = dstn;

#ifdef CONFIG_ZBAR_USE_STATIC_BUFFER
    dst->data = grey_buff;
#else
    dst->data = ZBAR_MALLOC(dst->datalen);
    if(!dst->data)
	{
		ZBAR_PRINT("Fail to alloc grey buff\n");
		return (-1);
	}
#endif

    unsigned char *dsty = (void*)dst->data;
    unsigned char flags = srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder;
	
    flags &= 2;
    const unsigned char *srcp = src->data;
    if(flags)
        srcp++;

    unsigned srcl = src->width + (src->width >> srcfmt->p.yuv.xsub2);
    unsigned x, y;
    unsigned char y0 = 0, y1 = 0;
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x += 2) {
            if(x < src->width) {
                y0 = *(srcp++);  srcp++;
                y1 = *(srcp++);  srcp++;
            }
            *(dsty++) = y0;
            *(dsty++) = y1;
        }
        if(x < src->width)
            srcp += (src->width - x) * 2;
    }

	return 0;
}

//scan one yuv422 image frame
static int scan_one_yuv422_image (unsigned char *yuy2_buf, unsigned int width, unsigned int height, char *output, unsigned int *output_count)
{
	int rc = 0;
	int found = 0;
	zbar_format_def_t *srcfmt = NULL;
	zbar_format_def_t *dstfmt = NULL;
	unsigned char *img_buff = yuy2_buf;

#if 0
	unsigned char *img_buff = (unsigned char *)ZBAR_MALLOC(width * height * 2);
	if(!img_buff)
		return -1;

	memcpy(img_buff, yuy2_buf, width * height * 2);
#endif

	zbar_image_t *srcimg = zbar_image_create();
	if(!srcimg)
	{
		ZBAR_PRINT("Fail to creat src image\n");
		return (-1);
	}

	zbar_image_t *dstimg = zbar_image_create();
	if(!dstimg)
	{
		ZBAR_PRINT("Fail to create dst image\n");
		rc = -1;
		goto error_dstimg;
	}

	/*
	Y800 Simple grayscale video
	Y422 Direct copy of UYVY as used by ADS Technologies Pyro WebCam firewire camera
	Y42B YUV 4:2:2 Planar
	*/
	zbar_image_set_format(srcimg, *(unsigned long*)"YUY2");
	zbar_image_set_size(srcimg, width, height);
	zbar_image_set_data(srcimg, img_buff, width*height*2, zbar_image_free_data);//zimage->data=blob;

	zbar_image_set_format(dstimg, *(unsigned long*)"Y800");//=="GREY" ??
	zbar_image_set_size(dstimg, width, height);

	srcfmt = (zbar_format_def_t *)_zbar_format_lookup(srcimg->format);//format_defs[]
	if(!srcfmt)
	{
		ZBAR_PRINT("Fail to lookup src fmt\n");
		rc = -1;
		goto error_exit;
	}
	dstfmt = (zbar_format_def_t *)_zbar_format_lookup(dstimg->format);
	if(!dstfmt)
	{
		ZBAR_PRINT("Fail to lookup dst fmt\n");
		rc = -1;
		goto error_exit;
	}

	if(zbar_convert_yuy2(dstimg, dstfmt, srcimg, srcfmt))
	{
		ZBAR_PRINT("Fail to convert YUY2 raw data to GREY\n");
		rc = -1;
		goto error_exit;
	}

	dstimg->cleanup = zbar_image_free_data;
	dstimg->refcnt = 1;
	dstimg->src = NULL;
	dstimg->srcidx = -1;
	dstimg->seq = 0;

	rc = zbar_process_image(processor, dstimg); //_zbar_process_image();
	if(rc<0)
	{
		rc = -1;
		goto error_exit;
	}

	// output result data
	int count = 0;
	const zbar_symbol_t *sym = zbar_image_first_symbol(dstimg);
	for(; sym; sym = zbar_symbol_next(sym)) {
		zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
		if(typ == ZBAR_PARTIAL)
			continue;
		else
		{
			char *pos = output+count;
			count += sprintf(pos, "%s%s:%s\n",
				zbar_get_symbol_name(typ),
				zbar_get_addon_name(typ),
				zbar_symbol_get_data(sym));

			found++;
		}
	}

	*output_count = count;

error_exit:
	zbar_symbol_set_ref(dstimg->syms, 0);
	if(dstimg->syms)
		_zbar_symbol_set_free(dstimg->syms);
	zbar_image_destroy(dstimg);
error_dstimg:
	srcimg->data = NULL;   //To avoid configASSERT() triggered in zbar_image_free_data(), let caller to free even this is a mallocated buffer.
	zbar_image_destroy(srcimg);

	if(rc<0)
		return rc;

	if(!found)
	{
		ZBAR_PRINT("No QRcode found\n");
		return (-1);
	}

    return(found);
}

int zbarimg_scan_one_yuv422_image(unsigned char *YUY2_buff, unsigned int width, unsigned int height, char *output, unsigned int *output_count)
{
	int rc=0;

	processor = zbar_processor_create(0);
	if(!processor)
	{
		ZBAR_PRINT("Fail to create processor \n");
		return -1;
	}

	if(zbar_processor_init(processor, NULL, 0))
	{
		ZBAR_PRINT("Fail to init processor \n");
		rc=-1;
		goto error;
	}

	rc = scan_one_yuv422_image(YUY2_buff, width, height, output, output_count);
	if(rc<0)
	{
		ZBAR_PRINT("Fail to scan image, rc=%d\n", rc);
		rc=-1;
		goto error;
	}

error:
	zbar_processor_destroy(processor);
	return rc;
}
#endif

