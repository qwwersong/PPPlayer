//
// Created by dell on 2019/02/26.
//

#ifndef SLPLAYER_PLAYSTATUS_H
#define SLPLAYER_PLAYSTATUS_H


class PlayStatus {

public:
    bool isExit = false;
    bool isLoading = true;
    bool isSeek = false;

public:
    PlayStatus();
    ~PlayStatus();

};


#endif //SLPLAYER_PLAYSTATUS_H
