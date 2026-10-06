#include "demoAVPushing_util.h"
int g_step = 0;
unsigned char* g_last_addr = NULL;

int getADTSFrame(unsigned char* buffer, unsigned int buf_size, unsigned char* frame_data, int* frame_size)
{
    int size = 0;
	//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "demo util %d: %p;%p;%p;%d\n",__LINE__,buffer,data,data_size,buf_size);

    if (!buffer || !frame_data || !frame_size)
	{
        return -1;
    }

    while (1)
	{
        if (buf_size < 7)
		{
			//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "demo util %d: 数据源过小 BB buffer size=%d\n",__LINE__,buf_size);
			return 1;
        }

        if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0))
		{

            size |= ((buffer[3] & 0x03) << 11);     //high 2 bit
            size |= buffer[4]<<3;                   //middle 8 bit
            size |= ((buffer[5] & 0xe0) >> 5);      //low 3 bitt

            break;
        }

        --buf_size;
        ++buffer;
    }

    if (buf_size < size)
	{
        return 1;
    }

    //copy buffer to data with size bits
    memcpy(frame_data, buffer, size);
    *frame_size = size;

	return 0;

}


int getH264Frame(unsigned char* buffer_start_addr, int buffer_size, unsigned char* frame_start_addr, int* frame_size, CONFIG_DATA *conf_data)
{

	unsigned char* tmp_mem = buffer_start_addr;

	int size = 0;

	if(conf_data->last_addr == NULL)
	{
		while(1)
		{
			if ((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01))
			{
				conf_data->last_addr = tmp_mem;
				break;
			}
			else if (((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01)))
			{
				conf_data->last_addr = tmp_mem;
				break;

			}

			if(tmp_mem > (buffer_start_addr+buffer_size-4))
			{
				OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "无法找到头");
				return 1;
			}
			tmp_mem += 1;
		}

	}


	tmp_mem = buffer_start_addr + conf_data->step;

	while(1)
	{

		if(conf_data->step >= buffer_size-4)
		{
			size = tmp_mem - conf_data->last_addr;
			OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "已读到文件尾,从头开始");
            if(size > *frame_size)
            {
                OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "frame too large %d\n",size);
                size= *frame_size;
            }
			*frame_size = size;
			memcpy(frame_start_addr, conf_data->last_addr, size);
			conf_data->last_addr = NULL;
			conf_data->step = 0 ;
			return 0;
		}
		unsigned char tmp_1 = 0;
		//NAL前的起始码00000001||000001
		if ((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01))
		{
			tmp_mem += 4;
			tmp_1 = (*(tmp_mem)&0x1f);

		}
		else if (((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01)))
		{
		    tmp_mem += 3;
			tmp_1 = (*(tmp_mem)&0x1f);
		}
		switch(tmp_1)
		{
			case (0x05):
				conf_data->cType = I_Frame;
				//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "I帧 %p , %d",tmp_mem,conf_data->step);
				break;
			case (0x01):case (0x02):
			case (0x03):case (0x04):
				conf_data->cType = P_Frame;
				//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "P帧 %p, %d",tmp_mem,conf_data->step);
				break;
			default:
				conf_data->cType = NONE;
				break;
		}


		if(conf_data->cType ==P_Frame || conf_data->cType ==I_Frame)
		{

			while(1)
			{
				if(conf_data->step >= buffer_size-4)
				{
					size = tmp_mem - conf_data->last_addr;
					OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "已读到文件尾,从头开始");
                    if(size > *frame_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "frame too large %d\n",size);
                        size= *frame_size;
                    }
					*frame_size = size;
					memcpy(frame_start_addr, conf_data->last_addr, size);
					conf_data->last_addr = NULL;
					conf_data->step = 0;
					return 0;
				}
				if ((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01)||((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01)))
				{
					size = tmp_mem - conf_data->last_addr;
					//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "size = %d",size);
					if(size > *frame_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "frame too large %d\n",size);
                        size= *frame_size;
                    }
					*frame_size = size;
					memcpy(frame_start_addr,conf_data->last_addr, size);
					conf_data->last_addr = tmp_mem;
					tmp_mem += 1;
					conf_data->step +=1;
					break;
				}
				else
				{
					tmp_mem += 1;
					conf_data->step +=1;
				}
			}
			break;
		}

		else
		{
			tmp_mem += 1;
			conf_data->step +=1;
		}

	}
	return 0;

}

int getH265Frame(unsigned char* buffer_start_addr, int buffer_size, unsigned char* frame_start_addr, int* frame_size, CONFIG_DATA *conf_data)
{

    unsigned char* tmp_mem = buffer_start_addr;

    int size = 0;

    if (conf_data->last_addr == NULL)
    {
        while (1)
        {
            if ((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x00) && (*(tmp_mem + 3) == 0x01))
            {
                conf_data->last_addr = tmp_mem;
                break;
            }
            else if (((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x01)))
            {
                conf_data->last_addr = tmp_mem;
                break;
            }

            if(tmp_mem > (buffer_start_addr + buffer_size - 4))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "无法找到头");
                return 1;
            }
            tmp_mem += 1;
        }
    }

    tmp_mem = buffer_start_addr + conf_data->step;

    while (1)
    {
        if(conf_data->step >= buffer_size - 4)
        {
            size = tmp_mem - conf_data->last_addr;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "已读到文件尾,从头开始");
            if (size > *frame_size)
            {
                OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "frame too large %d\n",size);
                size = *frame_size;
            }
            *frame_size = size;
            memcpy(frame_start_addr, conf_data->last_addr, size);
            conf_data->last_addr = NULL;
            conf_data->step = 0;
            return 0;
        }
        unsigned char tmp_1 = 0;
        //NAL前的起始码00000001||000001
        if ((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x00) && (*(tmp_mem + 3) == 0x01))
        {
            tmp_mem += 4;
            tmp_1 = (*(tmp_mem)) >> 1;
        }
        else if (((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x01)))
        {
            tmp_mem += 3;
            tmp_1 = (*(tmp_mem)) >> 1;
        }
        switch (tmp_1)
        {
            case 19:case 32:
            case 33:case 34:case 39:
                conf_data->cType = I_Frame;
                // OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "I帧 %p , %d",tmp_mem,conf_data->step);
                break;
			 case 1:
					  conf_data->cType = P_Frame;
					  break;
			default:
					  conf_data->cType = NONE;
					  break;

        }

        if (conf_data->cType == P_Frame || conf_data->cType == I_Frame)
        {
            while(1)
            {
                if  (conf_data->step >= buffer_size - 4)
                {
                    size = tmp_mem - conf_data->last_addr;
                    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "已读到文件尾,从头开始");
                    if (size > *frame_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "frame too large %d\n",size);
                        size = *frame_size;
                    }
                    *frame_size = size;
                    memcpy(frame_start_addr, conf_data->last_addr, size);
                    conf_data->last_addr = NULL;
                    conf_data->step = 0;
                    return 0;
                }

                #if 1
                //将sps pps vps idr为一帧发送
                if (conf_data->cType == I_Frame) {
                    tmp_1 = 0;
                    if ((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x00) && (*(tmp_mem + 3) == 0x01))
                    {
                        tmp_1 = (*(tmp_mem+4)) >> 1;
                        if ((19 == tmp_1) || (32 == tmp_1) || (33 == tmp_1) || (34 == tmp_1) || (39 == tmp_1)) {
                            tmp_mem += 4;
                            conf_data->step += 4;
                            continue;
                        }
                    }
                    else if (((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x01)))
                    {
                        tmp_1 = (*(tmp_mem+3)) >> 1;
                        if ((19 == tmp_1) || (32 == tmp_1) || (33 == tmp_1) || (34 == tmp_1) || (39 == tmp_1)) {
                            tmp_mem += 3;
                            conf_data->step += 3;
                            continue;
                        }
                    }
                }
                #endif

                if (((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) == 0x00) && (*(tmp_mem + 3) == 0x01)) ||
                    ((*(tmp_mem) == 0x00) && (*(tmp_mem + 1) == 0x00) && (*(tmp_mem + 2) ==0x01)))
                {
                    size = tmp_mem - conf_data->last_addr;
                    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "size = %d",size);
                    if (size > *frame_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "frame too large %d\n",size);
                        size = *frame_size;
                    }
                    *frame_size = size;
                    memcpy(frame_start_addr, conf_data->last_addr, size);
                    conf_data->last_addr = tmp_mem;
                    tmp_mem += 1;
                    conf_data->step +=1;
                    break;
                }
                else
                {
                    tmp_mem += 1;
                    conf_data->step += 1;
                }
            }
            break;
        }
        else
        {
            tmp_mem += 1;
            conf_data->step +=1;
        }
    }
    return 0;
}

