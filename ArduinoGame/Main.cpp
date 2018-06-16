
/* ArduinoGame */
/* Main.cpp */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <string>
#include <vector>

#define NOMINMAX
#include "DxLib.h"

template <typename T>
T Pi = static_cast<T>(3.141592653589793);

template <typename T>
inline T ConvertDegreeToRadian(T degreeValue)
{
    return degreeValue * Pi<T> / static_cast<T>(180.0);
}

template <typename T>
inline T ConvertRadianToDegree(T radianValue)
{
    return radianValue * static_cast<T>(180.0) / Pi<T>;
}

//
// CArduinoSerialInputクラス
// 以下のURLに掲載されていたソースコードを改変して使用
// https://blog.manash.me/serial-communication-with-an-arduino-using-c-on-windows-d08710186498
//
class CArduinoSerialInput
{
public:
    CArduinoSerialInput(const char* portName);
    ~CArduinoSerialInput();

    int Read(char* pBuffer, unsigned int bufferSize);
    inline bool IsConnected() const { return this->mIsConnected; }

private:
    HANDLE mHandle;
    COMSTAT mCommStatus;
    DWORD mError;
    bool mIsConnected;
};

CArduinoSerialInput::CArduinoSerialInput(const char* portName)
{
    // シリアルポートの接続
    this->mHandle = ::CreateFileA(
        static_cast<LPCSTR>(portName), GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (this->mHandle == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND)
            ::MessageBoxA(
                NULL, "Arduinoマイコンボードが接続されていません.",
                "Error", MB_OK | MB_ICONEXCLAMATION);
        else
            ::MessageBoxA(
                NULL, "Arduinoマイコンボードと接続できませんでした.",
                "Error", MB_OK | MB_ICONEXCLAMATION);

        this->mIsConnected = false;
        return;
    }

    // シリアルポートの現在のパラメータの取得
    DCB dcbSerialInputParameters;
    ZeroMemory(&dcbSerialInputParameters, sizeof(DCB));

    if (!::GetCommState(this->mHandle, &dcbSerialInputParameters)) {
        ::MessageBoxA(
            NULL, "シリアルポートの現在のパラメータの取得に失敗しました.",
            "Error", MB_OK | MB_ICONEXCLAMATION);
        this->mIsConnected = false;
        return;
    }

    // シリアルポートのパラメータの設定
    dcbSerialInputParameters.BaudRate = CBR_57600;
    dcbSerialInputParameters.ByteSize = 8;
    dcbSerialInputParameters.StopBits = ONESTOPBIT;
    dcbSerialInputParameters.Parity = NOPARITY;
    dcbSerialInputParameters.fDtrControl = DTR_CONTROL_ENABLE;

    if (!::SetCommState(this->mHandle, &dcbSerialInputParameters)) {
        ::MessageBoxA(
            NULL, "シリアルポートのパラメータの設定に失敗しました.",
            "Error", MB_OK | MB_ICONEXCLAMATION);
        this->mIsConnected = false;
        return;
    }

    // シリアルポートの接続の完了
    this->mIsConnected = true;
    ::PurgeComm(this->mHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    ::Sleep(20);
}

CArduinoSerialInput::~CArduinoSerialInput()
{
    if (this->mIsConnected) {
        ::CloseHandle(this->mHandle);
        this->mIsConnected = false;
    }
}

int CArduinoSerialInput::Read(char* pBuffer, unsigned int bufferSize)
{
    DWORD numOfBytesRead = 0;
    DWORD numOfBytesToRead = 0;
    DWORD bytesRead;
    
    ::ClearCommError(this->mHandle, &this->mError, &this->mCommStatus);

    if (this->mCommStatus.cbInQue > 0)
        if (this->mCommStatus.cbInQue > bufferSize)
            numOfBytesToRead = bufferSize;
        else
            numOfBytesToRead = this->mCommStatus.cbInQue;

    for (DWORD i = 0; i < numOfBytesToRead; ++i) {
        if (!::ReadFile(this->mHandle, &pBuffer[i], 1, &bytesRead, NULL))
            return numOfBytesRead;

        ++numOfBytesRead;

        // 改行コードを検出したら読み込みを終える
        if (pBuffer[i] == '\n')
            return numOfBytesRead;
    }

    return 0;
}

//
// PipeObject構造体
//
struct PipeObject
{
    int mPositionX;             // 土管の左端のX座標
    int mTopPipeHeight;         // 上側の土管の高さ
    int mBottomPipeHeight;      // 下側の土管の高さ
    bool mPassed;               // 土管を通過したかどうか
};

//
// GameState列挙体
//
enum class GameState
{
    Start,
    Play,
    GameOver
};

//
// CGameクラス
//
class CGame final
{
public:
    static CGame* GetInstance();
    
    CGame(const CGame&) = delete;
    CGame(CGame&&) = delete;
    CGame& operator=(const CGame&) = delete;
    CGame& operator=(CGame&&) = delete;

    int Run();

private:
    CGame();
    ~CGame() = default;

    bool InitializeArduinoInput();
    void FinalizeArduinoInput();
    void HandleInput();
    void LoadImages();
    void LoadFonts();
    void InitializeParameters();
    void Update();
    void Draw();

private:
    CArduinoSerialInput* mArduinoInput;     // シリアルポートの入力
    char* mSerialInputBuffer;               // 入力バッファ
    double mInputValue;                     // センサの入力値
    double mOldInputValue;                  // 1つ前のセンサの入力値

    GameState mGameState;                   // ゲームの状態

    int mImageHandleBackground;             // 背景画像のハンドル
    int mImageHandleBird[3];                // 鳥画像のハンドル
    int mImageHandleGround;                 // 地面画像のハンドル
    int mImageHandlePipe;                   // 土管画像のハンドル
    int mImageHandleRestart;                // リスタート画像のハンドル
    int mImageHandleScore;                  // スコア表示画像のハンドル

    int mImageBackgroundWidth;              // 背景画像の横幅
    int mImageBackgroundHeight;             // 背景画像の縦幅
    int mImageGroundWidth;                  // 地面画像の横幅
    int mImageGroundHeight;                 // 地面画像の縦幅
    int mImageGroundOffset;                 // 地面画像のスクロール量
    int mNumOfGroundImages;                 // 地面画像の描画個数

    int mImagePipeWidth;                    // 土管画像の横幅
    int mImagePipeHeight;                   // 土管画像の縦幅

    int mImageScoreWidth;                   // スコア表示画像の横幅
    int mImageScoreHeight;                  // スコア表示画像の縦幅

    int mBirdWidth;                         // 鳥画像の横幅
    int mBirdHeight;                        // 鳥画像の縦幅
    int mBirdPositionX;                     // 鳥の左上X座標
    int mBirdPositionY;                     // 鳥の左上Y座標
    int mBirdPositionMaxY;                  // 鳥の左上Y座標の最大値

    int mGamePlayThresholdPositionY;        // プレイ画面へ遷移するために必要な鳥の左上Y座標の閾値
    int mPipeGenerateCounter;               // 土管の出現カウンタ
    int mPipeGenerateCounterThreshold;      // 土管を出現させるために必要な土管のカウンタ値の閾値
    int mPipeGap;                           // 上下の土管の間隔
    std::vector<PipeObject> mPipeObjects;   // 土管オブジェクトのリスト

    int mFontHandle;                        // フォントのハンドル

    int mScore;                             // スコア
    int mBestScore;                         // ベストスコア
    double mRestartThresholdPositionY;      // リスタートするために必要なセンサの入力値の閾値

    static const char* ApplicationName;     // アプリケーション名
    static const int WindowWidth;           // ウィンドウの横幅
    static const int WindowHeight;          // ウィンドウの縦幅
    static const int ColorBitDepth;         // カラービット数
    static const int RefreshRate;           // フレームレート
    static const char* PortName;            // 接続ポート名
    static const int SerialInputBufferSize; // 入力バッファのサイズ
};

const char* CGame::ApplicationName = "ArduinoGame";     // アプリケーション名
const int CGame::WindowWidth = 768;                     // ウィンドウの横幅
const int CGame::WindowHeight = 1024;                   // ウィンドウの縦幅
const int CGame::ColorBitDepth = 32;                    // カラービット数
const int CGame::RefreshRate = 60;                      // フレームレート
const char* CGame::PortName = "\\\\.\\COM3";            // 接続ポート名
const int CGame::SerialInputBufferSize = 256;           // 入力バッファのサイズ

CGame* CGame::GetInstance()
{
    static CGame theInstance;
    return &theInstance;
}

CGame::CGame() :
    mArduinoInput(nullptr),
    mSerialInputBuffer(nullptr),
    mInputValue(0.0),
    mOldInputValue(0.0),
    mGameState(GameState::Start),
    mImageHandleBackground(0),
    mImageHandleBird(),
    mImageHandleGround(0),
    mImageHandlePipe(0),
    mImageHandleRestart(0),
    mImageHandleScore(0),
    mImageBackgroundWidth(0),
    mImageBackgroundHeight(0),
    mImageGroundWidth(0),
    mImageGroundHeight(0),
    mImageGroundOffset(0),
    mNumOfGroundImages(0),
    mImagePipeWidth(0),
    mImagePipeHeight(0),
    mImageScoreWidth(0),
    mImageScoreHeight(0),
    mBirdWidth(0),
    mBirdHeight(0),
    mBirdPositionX(0),
    mBirdPositionY(0),
    mBirdPositionMaxY(0),
    mGamePlayThresholdPositionY(0),
    mPipeGenerateCounter(0),
    mPipeGenerateCounterThreshold(0),
    mPipeGap(0),
    mPipeObjects(),
    mFontHandle(0),
    mScore(0),
    mBestScore(0),
    mRestartThresholdPositionY(0.0)
{
}

bool CGame::InitializeArduinoInput()
{
    // 入力バッファの確保
    this->mSerialInputBuffer = new char[CGame::SerialInputBufferSize];

    if (this->mSerialInputBuffer == nullptr)
        return false;

    // シリアルポート接続の初期化
    this->mArduinoInput = new CArduinoSerialInput(CGame::PortName);

    if (this->mArduinoInput == nullptr) {
        ::MessageBoxA(
            NULL, "Arduinoマイコンボードとの接続に失敗しました.",
            CGame::ApplicationName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    if (!this->mArduinoInput->IsConnected()) {
        ::MessageBoxA(
            NULL, "Arduinoマイコンボードとの接続に失敗しました.",
            CGame::ApplicationName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    ::MessageBoxA(
        NULL, "Arduinoマイコンボードとの接続が確立されました.",
        CGame::ApplicationName, MB_OK | MB_ICONINFORMATION);

    return true;
}

void CGame::FinalizeArduinoInput()
{
    if (this->mSerialInputBuffer != nullptr) {
        delete[] this->mSerialInputBuffer;
        this->mSerialInputBuffer = nullptr;
    }

    if (this->mArduinoInput != nullptr) {
        delete this->mArduinoInput;
        this->mArduinoInput = nullptr;
    }
}

void CGame::HandleInput()
{
    // シリアルポートからデータを取得
    int bytesRead = this->mArduinoInput->Read(
        this->mSerialInputBuffer, CGame::SerialInputBufferSize);
    
    if (!bytesRead)
        return;

    this->mSerialInputBuffer[bytesRead] = '\0';

    // センサの出力値のデバッグ出力
    // ::OutputDebugStringA(static_cast<LPCSTR>(this->mSerialInputBuffer));

    // センサの出力値の読み込み
    int inputValue;
    int inputValueDummy;

    if (sscanf_s(this->mSerialInputBuffer, "%d,%d", &inputValue, &inputValueDummy) != 2)
        return;

    // 指数重み付き移動平均の計算
    this->mInputValue = 0.95 * this->mInputValue + 0.05 * static_cast<double>(inputValue);
}

void CGame::LoadImages()
{
    // 画像の読み込み
    this->mImageHandleBackground = DxLib::LoadGraph("Images/Background.png");
    this->mImageHandleGround = DxLib::LoadGraph("Images/Ground.png");
    this->mImageHandlePipe = DxLib::LoadGraph("Images/Pipe.png");
    this->mImageHandleRestart = DxLib::LoadGraph("Images/Restart.png");
    this->mImageHandleScore = DxLib::LoadGraph("Images/Score.png");

    // キャラクターの画像の分割読み込み
    this->mBirdWidth = 92;
    this->mBirdHeight = 64;
    DxLib::LoadDivGraph(
        "Images/Bird.png", 3, 3, 1,
        this->mBirdWidth, this->mBirdHeight, this->mImageHandleBird);

    // 背景画像のサイズを取得
    DxLib::GetGraphSize(
        this->mImageHandleBackground,
        &this->mImageBackgroundWidth, &this->mImageBackgroundHeight);

    // 地面画像のサイズを取得
    DxLib::GetGraphSize(
        this->mImageHandleGround,
        &this->mImageGroundWidth, &this->mImageGroundHeight);

    // 土管画像のサイズを取得
    DxLib::GetGraphSize(
        this->mImageHandlePipe,
        &this->mImagePipeWidth, &this->mImagePipeHeight);

    // スコア表示画像のサイズを取得
    DxLib::GetGraphSize(
        this->mImageHandleScore,
        &this->mImageScoreWidth, &this->mImageScoreHeight);
}

void CGame::LoadFonts()
{
    // フォントの初期化
    this->mFontHandle = DxLib::CreateFontToHandle("Consolas", 48, -1, DX_FONTTYPE_ANTIALIASING_4X4);
}

void CGame::InitializeParameters()
{
    // 地面画像の個数を計算
    this->mNumOfGroundImages = this->mImageBackgroundWidth / this->mImageGroundWidth + 2;

    // キャラクターの横方向の座標を計算
    this->mBirdPositionX =
        static_cast<int>(static_cast<double>(CGame::WindowWidth) * 0.4) - this->mBirdWidth / 2;

    // キャラクターの座標の最大値を計算
    this->mBirdPositionMaxY =
        CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight;

    // プレイ画面へ遷移するために必要なキャラクターの座標の閾値を計算
    this->mGamePlayThresholdPositionY =
        CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight - 100;

    // 土管を出現させるために必要なカウンタの閾値を計算
    this->mPipeGenerateCounterThreshold =
        static_cast<int>(static_cast<double>(this->mImagePipeWidth) * 1.2);

    // 土管の上下の間隔を計算
    this->mPipeGap = static_cast<int>(static_cast<double>(CGame::WindowWidth) * 0.3);

    // リスタートするために必要なセンサの入力値の閾値の設定
    this->mRestartThresholdPositionY = 500.0;
}

void CGame::Update()
{
    // 地面の移動
    this->mImageGroundOffset = (this->mImageGroundOffset + 4) % this->mImageGroundWidth;

    switch (this->mGameState) {
        case GameState::Start:
        {
            // キャラクターの位置の計算
            this->mBirdPositionY = std::min(
                this->mBirdPositionMaxY,
                CGame::WindowHeight - static_cast<int>(this->mInputValue) - this->mBirdHeight);

            // キャラクターの座標が閾値を下回ったらプレイ画面へ遷移
            if (this->mBirdPositionY < this->mGamePlayThresholdPositionY)
                this->mGameState = GameState::Play;

            break;
        }

        case GameState::Play:
        {
            // キャラクターの位置の計算
            this->mBirdPositionY = std::min(
                this->mBirdPositionMaxY,
                CGame::WindowHeight - static_cast<int>(this->mInputValue) - this->mBirdHeight);

            // 土管の移動
            for (auto& pipeObject : this->mPipeObjects)
                pipeObject.mPositionX -= 4;

            // 画面からはみ出た土管を除去
            this->mPipeObjects.erase(
                std::remove_if(
                    this->mPipeObjects.begin(),
                    this->mPipeObjects.end(),
                    [this](const PipeObject& pipeObject) {
                        return pipeObject.mPositionX < -this->mImagePipeWidth;
                }),
                this->mPipeObjects.end());

            // 土管の出現カウンタの更新
            this->mPipeGenerateCounter++;

            // 土管の出現カウンタが一定値を上回ったら出現させる
            if (this->mPipeGenerateCounter >= this->mPipeGenerateCounterThreshold) {
                this->mPipeGenerateCounter = 0;

                // 新しい土管オブジェクトの作成
                PipeObject pipeObject;
                pipeObject.mPositionX = CGame::WindowWidth + 128;
                pipeObject.mTopPipeHeight =
                    DxLib::GetRand(CGame::WindowHeight - this->mImageGroundHeight - this->mPipeGap);
                pipeObject.mBottomPipeHeight =
                    CGame::WindowHeight - this->mImageGroundHeight -
                    pipeObject.mTopPipeHeight - this->mPipeGap;
                pipeObject.mPassed = false;

                // 新しい土管オブジェクトの追加
                this->mPipeObjects.push_back(pipeObject);
            }

            // 土管とキャラクタの衝突の検知
            auto collidePipeIterator = std::find_if(
                this->mPipeObjects.begin(), this->mPipeObjects.end(),
                [this](const PipeObject& pipeObject) {
                    // 土管との衝突判定(横方向)
                    if (this->mBirdPositionX < pipeObject.mPositionX + this->mImagePipeWidth &&
                        this->mBirdPositionX + this->mBirdWidth > pipeObject.mPositionX)
                        // 上側の土管との衝突判定(縦方向)
                        if (this->mBirdPositionY < pipeObject.mTopPipeHeight &&
                            this->mBirdPositionY + this->mBirdHeight > 0.0)
                            return true;
                        // 下側の土管との衝突判定(縦方向)
                        else if (this->mBirdPositionY < CGame::WindowHeight - this->mImageGroundHeight &&
                            this->mBirdPositionY + this->mBirdHeight >
                            CGame::WindowHeight - this->mImageGroundHeight - pipeObject.mBottomPipeHeight)
                            return true;

                    return false;
            });
            
            // 衝突したらゲームオーバー画面へ遷移
            if (collidePipeIterator != this->mPipeObjects.end()) {
                this->mGameState = GameState::GameOver;

                // ベストスコアと比較して現在のスコアの方が大きければ更新
                this->mBestScore = std::max(this->mBestScore, this->mScore);

                // センサの入力値のクリア
                this->mInputValue = 0.0;
                this->mOldInputValue = 0.0;
            }

            // 土管をキャラクタが通り抜けたかどうかを調べて, 新たに通り抜けた土管があればスコアを加算
            std::for_each(
                this->mPipeObjects.begin(),
                this->mPipeObjects.end(),
                [this](PipeObject& pipeObject) {
                    if (!pipeObject.mPassed &&
                        pipeObject.mPositionX + this->mImagePipeWidth <= this->mBirdPositionX) {
                        pipeObject.mPassed = true;
                        this->mScore++;
                    }
                });

            break;
        }

        case GameState::GameOver:
        {
            // キャラクターの位置の計算(土管に衝突した後に地面に落下)
            this->mBirdPositionY = std::min(
                this->mBirdPositionY + 32,
                CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight);

            // センサの入力値が閾値を超えたらリスタート
            if (this->mInputValue > this->mRestartThresholdPositionY) {
                this->mGameState = GameState::Start;

                // センサの入力値のクリア
                this->mInputValue = 0.0;
                this->mOldInputValue = 0.0;

                // 土管の出現カウンタのリセット
                this->mPipeGenerateCounter = 0;

                // 土管オブジェクトのリストのクリア
                this->mPipeObjects.clear();

                // スコアのリセット
                this->mScore = 0;
            }
            break;
        }
    }
}

void CGame::Draw()
{
    // デバッグ出力
    /*
    DxLib::clsDx();
    DxLib::printfDx("%s\n",
        (this->mGameState == GameState::Start) ? "GameState::Start" :
        (this->mGameState == GameState::Play) ? "GameState::Play" :
        (this->mGameState == GameState::GameOver) ? "GameState::GameOver" : "Unknown");
    DxLib::printfDx("%u", this->mPipeObjects.size());
    */

    // 背景画像の描画
    DxLib::DrawGraph(0, 0, this->mImageHandleBackground, FALSE);

    switch (this->mGameState) {
        case GameState::Start:
        {
            // キャラクターの描画
            DxLib::DrawGraph(
                this->mBirdPositionX,
                this->mBirdPositionY,
                this->mImageHandleBird[0], TRUE);
            break;
        }

        case GameState::Play:
        {
            // 土管の描画
            for (const auto& pipeObject : this->mPipeObjects) {
                // 土管の上側の部分
                DxLib::DrawRotaGraph(
                    pipeObject.mPositionX + this->mImagePipeWidth / 2,
                    pipeObject.mTopPipeHeight - this->mImagePipeHeight / 2,
                    1.0, ConvertDegreeToRadian<double>(180.0),
                    this->mImageHandlePipe, FALSE, FALSE, FALSE);

                // 土管の下側の部分
                DxLib::DrawGraph(
                    pipeObject.mPositionX,
                    CGame::WindowHeight - this->mImageGroundHeight - pipeObject.mBottomPipeHeight,
                    this->mImageHandlePipe, FALSE);
            }

            // キャラクターの描画
            DxLib::DrawGraph(
                this->mBirdPositionX,
                this->mBirdPositionY,
                this->mImageHandleBird[0], TRUE);

            // スコアの描画
            std::string scoreText = std::to_string(this->mScore);
            int scoreTextWidth = DxLib::GetDrawStringWidthToHandle(
                static_cast<const TCHAR*>(scoreText.c_str()),
                static_cast<int>(scoreText.size()),
                this->mFontHandle);
            DxLib::DrawStringToHandle(
                (CGame::WindowWidth - scoreTextWidth) / 2,
                static_cast<int>(static_cast<double>(CGame::WindowHeight) * 0.2),
                static_cast<const TCHAR*>(scoreText.c_str()),
                DxLib::GetColor(255, 0, 0),
                this->mFontHandle);

            break;
        }

        case GameState::GameOver:
        {
            // キャラクターの描画
            DxLib::DrawRotaGraph(
                this->mBirdPositionX - this->mBirdWidth / 2,
                this->mBirdPositionY + this->mBirdHeight / 2,
                1.0, ConvertDegreeToRadian<double>(90.0),
                this->mImageHandleBird[0], TRUE, FALSE, FALSE);

            // スコアの表示画像の描画
            DxLib::DrawGraph(
                (CGame::WindowWidth - this->mImageScoreWidth) / 2,
                static_cast<int>(static_cast<double>(CGame::WindowHeight) * 0.2),
                this->mImageHandleScore, TRUE);

            // スコアの描画
            std::string scoreText = std::to_string(this->mScore);
            std::string bestScoreText = std::to_string(this->mBestScore);
            int scoreTextWidth = DxLib::GetDrawStringWidthToHandle(
                static_cast<const TCHAR*>(scoreText.c_str()),
                static_cast<int>(scoreText.size()),
                this->mFontHandle);
            int bestScoreTextWidth = DxLib::GetDrawStringWidthToHandle(
                static_cast<const TCHAR*>(bestScoreText.c_str()),
                static_cast<int>(bestScoreText.size()),
                this->mFontHandle);

            DxLib::DrawStringToHandle(
                (CGame::WindowWidth - scoreTextWidth) / 2,
                static_cast<int>(static_cast<double>(CGame::WindowHeight) * 0.26),
                static_cast<const TCHAR*>(scoreText.c_str()),
                DxLib::GetColor(255, 0, 0),
                this->mFontHandle);
            DxLib::DrawStringToHandle(
                (CGame::WindowWidth - bestScoreTextWidth) / 2,
                static_cast<int>(static_cast<double>(CGame::WindowHeight) * 0.34),
                static_cast<const TCHAR*>(bestScoreText.c_str()),
                DxLib::GetColor(255, 0, 0),
                this->mFontHandle);

            break;
        }
    }

    // 地面画像の描画
    for (int i = 0; i < this->mNumOfGroundImages; ++i) {
        DxLib::DrawGraph(
            this->mImageGroundWidth * i - this->mImageGroundOffset,
            this->mImageBackgroundHeight,
            this->mImageHandleGround, FALSE);
    }
}

int CGame::Run()
{
    // シリアルポート接続の初期化
    this->InitializeArduinoInput();

    // Dxライブラリの初期化
    DxLib::ChangeWindowMode(TRUE);
    DxLib::SetGraphMode(
        CGame::WindowWidth, CGame::WindowHeight,
        CGame::ColorBitDepth, CGame::RefreshRate);
    DxLib::SetWindowSize(CGame::WindowWidth, CGame::WindowHeight);
    DxLib::SetMainWindowText(CGame::ApplicationName);
    DxLib::SetOutApplicationLogValidFlag(FALSE);

    if (DxLib::DxLib_Init() == -1)
        return -1;

    // 画像の読み込み
    this->LoadImages();

    // フォントの読み込み
    this->LoadFonts();

    // パラメータの初期化
    this->InitializeParameters();

    while (DxLib::ProcessMessage() == 0) {
        DxLib::ClearDrawScreen();
        DxLib::SetDrawScreen(DX_SCREEN_BACK);

        // センサからの電圧値を取得
        this->HandleInput();

        // 更新処理
        this->Update();

        // 描画処理
        this->Draw();

        DxLib::ScreenFlip();
    }

    // Dxライブラリの終了処理
    DxLib::DxLib_End();

    // シリアルポート接続の終了処理
    this->FinalizeArduinoInput();

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return CGame::GetInstance()->Run();
}
