/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "GuiIconGrid.h"
#include "GuiController.h"
#include "common/common.h"
#include "Application.h"
#include "video/CVideo.h"
#include "game/GameList.h"

GuiIconGrid::GuiIconGrid(int w, int h)
    : GuiGameBrowser(w, h)
    , buttonClickSound(Resources::GetSound("button_click.mp3"))
    , noIcon(Resources::GetFile("noGameIcon.png"), Resources::GetFileSize("noGameIcon.png"), GX2_TEX_CLAMP_MIRROR)
    , emptyIcon(Resources::GetFile("iconEmpty.jpg"), Resources::GetFileSize("iconEmpty.jpg"), GX2_TEX_CLAMP_MIRROR)
    , particleBgImage(w, h, 50)
    , touchTrigger(GuiTrigger::CHANNEL_1, GuiTrigger::VPAD_TOUCH)
    , leftTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_LEFT, true)
    , rightTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_RIGHT, true)
    , downTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_DOWN, true)
    , upTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_UP, true)
    , buttonATrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_A, true)
    , buttonLTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_L, true)
    , buttonRTrigger(GuiTrigger::CHANNEL_ALL, GuiTrigger::BUTTON_R, true)
    , leftButton(w, h)
    , rightButton(w, h)
    , downButton(w, h)
    , upButton(w, h)
    , launchButton(w, h)
    , arrowRightImageData(Resources::GetImageData("rightArrow.png"))
    , arrowLeftImageData(Resources::GetImageData("leftArrow.png"))
    , arrowRightImage(arrowRightImageData)
    , arrowLeftImage(arrowLeftImageData)
    , arrowRightButton(arrowRightImage.getWidth(), arrowRightImage.getHeight())
    , arrowLeftButton(arrowLeftImage.getWidth(), arrowLeftImage.getHeight())
{
    listOffset = 0;
    gameLaunchTimer = 0;
    selectedGame = 0;
    currentLeftPosition = 0;
    targetLeftPosition = 0;

    particleBgImage.setParent(this);

    leftButton.setTrigger(&leftTrigger);
    leftButton.clicked.connect(this, &GuiIconGrid::OnLeftClick);
    this->append(&leftButton);

    rightButton.setTrigger(&rightTrigger);
    rightButton.clicked.connect(this, &GuiIconGrid::OnRightClick);
    this->append(&rightButton);

    downButton.setTrigger(&downTrigger);
    downButton.clicked.connect(this, &GuiIconGrid::OnDownClick);
    this->append(&downButton);

    upButton.setTrigger(&upTrigger);
    upButton.clicked.connect(this, &GuiIconGrid::OnUpClick);
    this->append(&upButton);

    launchButton.setTrigger(&buttonATrigger);
    launchButton.clicked.connect(this, &GuiIconGrid::OnLaunchClick);
    this->append(&launchButton);

    int maxPages = (GameList::instance()->size() + (MAX_COLS * MAX_ROWS - 1)) / (MAX_COLS * MAX_ROWS);

    for(int idx = 0; idx < (maxPages * MAX_COLS * MAX_ROWS); idx++)
    {
        GameIcon *image = NULL;
        if(idx < GameList::instance()->size())
        {
            std::string filepath = GameList::instance()->at(idx)->gamepath + META_PATH + "/iconTex.tga";
            image = new GameIcon(filepath, &noIcon);
        }
        else
        {
            image = new GameIcon("", &emptyIcon);
        }

        image->setRenderReflection(false);
        image->setStrokeRender(false);
        image->setSelected(idx == selectedGame);
        image->setRenderIconLast(true);

        GuiButton * button = new GuiButton(noIcon.getWidth(), noIcon.getHeight());
        button->setImage(image);
        button->setPosition(0, 0);
        button->setEffectGrow();
        button->setTrigger(&touchTrigger);
        button->setSoundClick(buttonClickSound);
        button->setClickable( (idx < GameList::instance()->size()) );
        button->setSelectable( (idx < GameList::instance()->size()) );
        button->clicked.connect(this, &GuiIconGrid::OnGameButtonClick);
        this->append(button);

        gameButtons.push_back(button);
        gameIcons.push_back(image);
    }

    updateButtonPositions();

    if((MAX_ROWS * MAX_COLS) < GameList::instance()->size())
    {
        arrowLeftButton.setImage(&arrowLeftImage);
        arrowLeftButton.setEffectGrow();
        arrowLeftButton.setPosition(40, 0);
        arrowLeftButton.setAlignment(ALIGN_LEFT | ALIGN_MIDDLE);
        arrowLeftButton.setTrigger(&touchTrigger, 0);
        arrowLeftButton.setTrigger(&buttonLTrigger, 1);
        arrowLeftButton.clicked.connect(this, &GuiIconGrid::OnLeftArrowClick);

        arrowRightButton.setImage(&arrowRightImage);
        arrowRightButton.setEffectGrow();
        arrowRightButton.setPosition(-40, 0);
        arrowRightButton.setAlignment(ALIGN_RIGHT | ALIGN_MIDDLE);
        arrowRightButton.setTrigger(&touchTrigger, 0);
        arrowRightButton.setTrigger(&buttonRTrigger, 1);
        arrowRightButton.clicked.connect(this, &GuiIconGrid::OnRightArrowClick);
        append(&arrowRightButton);
    }
}

GuiIconGrid::~GuiIconGrid()
{
    for(u32 i = 0; i < gameButtons.size(); i++)
    {
        delete gameIcons[i];
        delete gameButtons[i];
    }

    Resources::RemoveImageData(arrowRightImageData);
    Resources::RemoveImageData(arrowLeftImageData);
    Resources::RemoveSound(buttonClickSound);
}

void GuiIconGrid::setSelectedGame(int idx)
{
    for(u32 i = 0; i < (u32)GameList::instance()->size(); i++)
    {
        if(i == (u32)idx)
        {
            selectedGame = idx;

            while(selectedGame > listOffset * MAX_COLS * MAX_ROWS)
                listOffset++;

            while(selectedGame < listOffset * MAX_COLS * MAX_ROWS)
                listOffset--;

            targetLeftPosition = -listOffset * getWidth();

            int maxPages = (GameList::instance()->size() + (MAX_COLS * MAX_ROWS - 1)) / (MAX_COLS * MAX_ROWS);
            if(listOffset == 0)
            {
                append(&arrowRightButton);
                remove(&arrowLeftButton);
            }
            else if(listOffset >= (maxPages - 1))
            {
                append(&arrowLeftButton);
                remove(&arrowRightButton);
            }
            else
            {
                append(&arrowLeftButton);
                append(&arrowRightButton);
            }
        }

        gameIcons[i]->setSelected((u32)idx == i);
    }
}

int GuiIconGrid::getSelectedGame(void)
{
    return selectedGame;
}

void GuiIconGrid::OnLeftArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    if(listOffset > 0)
    {
        listOffset--;
        targetLeftPosition = -listOffset * getWidth();

        if(listOffset == 0)
            remove(&arrowLeftButton);

        int sel = getSelectedGame();
        sel -= MAX_ROWS * MAX_COLS;
        if(sel < 0)
            sel = 0;

        if(sel != getSelectedGame())
        {
            setSelectedGame(sel);
            gameSelectionChanged(this, sel);
        }
    }

    append(&arrowRightButton);
}

void GuiIconGrid::OnRightArrowClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    int maxPages = (GameList::instance()->size() + (MAX_COLS * MAX_ROWS - 1)) / (MAX_COLS * MAX_ROWS);

    if(listOffset < (maxPages-1))
    {
        listOffset++;
        targetLeftPosition = -listOffset * getWidth();

        if(listOffset == (maxPages-1))
            remove(&arrowRightButton);

        int sel = getSelectedGame();
        sel += MAX_ROWS * MAX_COLS;
        if(sel > (GameList::instance()->size() - 1))
            sel = (GameList::instance()->size() - 1);

        if(sel != getSelectedGame())
        {
            setSelectedGame(sel);
            gameSelectionChanged(this, sel);
        }
    }

    append(&arrowLeftButton);
}

void GuiIconGrid::OnLeftClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    int sel = getSelectedGame();
    int col = sel % MAX_COLS;

    if(col == 0 && listOffset > 0)
    {
        listOffset--;
        targetLeftPosition = -listOffset * getWidth();
        sel += MAX_COLS - 1 - MAX_ROWS * MAX_COLS;

        if(listOffset == 0)
            remove(&arrowLeftButton);
        append(&arrowRightButton);
    }
    else if(col > 0)
    {
        sel--;
    }

    if(sel != getSelectedGame())
    {
        setSelectedGame(sel);
        gameSelectionChanged(this, sel);
    }
}

void GuiIconGrid::OnRightClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    int sel = getSelectedGame();
    int col = sel % MAX_COLS;

    if(col == (MAX_COLS - 1))
    {
        int maxPages = (GameList::instance()->size() + (MAX_COLS * MAX_ROWS - 1)) / (MAX_COLS * MAX_ROWS);

        if(((sel + MAX_ROWS * MAX_COLS - MAX_COLS + 1) < GameList::instance()->size()) || (listOffset < (maxPages-1)))
        {
            listOffset++;
            targetLeftPosition = -listOffset * getWidth();
            sel += MAX_ROWS * MAX_COLS - MAX_COLS + 1;

            if(listOffset < (maxPages-1))
                remove(&arrowRightButton);
            append(&arrowLeftButton);
        }
    }
    else
    {
        sel++;
    }

    if(sel > (GameList::instance()->size() - 1))
        sel = (GameList::instance()->size() - 1);

    if(sel != getSelectedGame())
    {
        setSelectedGame(sel);
        gameSelectionChanged(this, sel);
    }
}

void GuiIconGrid::OnDownClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    int sel = getSelectedGame();
    int row = (sel % (MAX_ROWS * MAX_COLS)) / MAX_COLS;

    if(row < (MAX_ROWS - 1))
    {
        sel += MAX_COLS;
    }

    if(sel > (GameList::instance()->size() - 1))
        sel = (GameList::instance()->size() - 1);

    if(sel != getSelectedGame())
    {
        setSelectedGame(sel);
        gameSelectionChanged(this, sel);
    }
}
void GuiIconGrid::OnUpClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    int sel = getSelectedGame();
    int row = (sel % (MAX_ROWS * MAX_COLS)) / MAX_COLS;

    if(row > 0)
    {
        sel -= MAX_COLS;
    }

    if(sel < 0)
        sel = 0;

    if(sel != getSelectedGame())
    {
        setSelectedGame(sel);
        gameSelectionChanged(this, sel);
    }
}

void GuiIconGrid::OnGameButtonClick(GuiButton *button, const GuiController *controller, GuiTrigger *trigger)
{
    for(u32 i = 0; i < gameButtons.size(); ++i)
    {
        if(gameButtons[i] == button && ((int)i < GameList::instance()->size()))
        {
            if(selectedGame == (int)i)
            {
                if(gameLaunchTimer < 30)
                    OnLaunchClick(button, controller, trigger);
            }
            else
            {
                setSelectedGame(i);
                gameSelectionChanged(this, selectedGame);
            }
            gameLaunchTimer = 0;
            break;
        }
    }
}

void GuiIconGrid::updateButtonPositions()
{
    int col = 0, row = 0, listOff = 0;

    for(u32 i = 0; i < gameButtons.size(); i++)
    {
        listOff = i / (MAX_COLS * MAX_ROWS);

        float posX = currentLeftPosition + listOff * width + ( col * (noIcon.getWidth() + noIcon.getWidth() * 0.5f) - (MAX_COLS * 0.5f - 0.5f) * (noIcon.getWidth() + noIcon.getWidth() * 0.5f) );
        float posY = -row * (noIcon.getHeight() + noIcon.getHeight() * 0.5f) + (MAX_ROWS * 0.5f - 0.5f) * (noIcon.getHeight() + noIcon.getHeight() * 0.5f) + 30.0f;

        gameButtons[i]->setPosition(posX, posY);

        col++;
        if(col >= MAX_COLS)
        {
            col = 0;
            row++;
        }
        if(row >= MAX_ROWS)
            row = 0;
    }
}

void GuiIconGrid::update(GuiController * c)
{
    GuiFrame::update(c);
}

void GuiIconGrid::draw(CVideo *pVideo)
{
    bool bUpdatePositions = false;

    if(currentLeftPosition < targetLeftPosition)
    {
        currentLeftPosition += 35;

        if(currentLeftPosition > targetLeftPosition)
            currentLeftPosition = targetLeftPosition;

        bUpdatePositions = true;
    }
    else if(currentLeftPosition > targetLeftPosition)
    {
        currentLeftPosition -= 35;

        if(currentLeftPosition < targetLeftPosition)
            currentLeftPosition = targetLeftPosition;

        bUpdatePositions = true;
    }

    if(bUpdatePositions)
    {
        bUpdatePositions = false;
        updateButtonPositions();
    }


    //! the BG needs to be rendered to stencil
    pVideo->setStencilRender(true);
    particleBgImage.draw(pVideo);
    pVideo->setStencilRender(false);

    GuiFrame::draw(pVideo);

    gameLaunchTimer++;
}
