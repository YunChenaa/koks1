#pragma once

struct fast_frame_ev_info {
    int isReady;

    int irAeIntegrationTime0;
    int irAeIntegrationTime1;
    int irAeAGain1;
    int irAeIspDGain1;

    int rgbAeIntegrationTime;
    int rgbAeAGain;
    int rgbAeIspDGain;

    int luma;
    int ev;
};