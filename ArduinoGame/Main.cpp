
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
// CArduinoSerialInput�N���X
// �ȉ���URL�Ɍf�ڂ���Ă����\�[�X�R�[�h�����ς��Ďg�p
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
    // �V���A���|�[�g�̐ڑ�
    this->mHandle = ::CreateFileA(
        static_cast<LPCSTR>(portName), GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (this->mHandle == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND)
            ::MessageBoxA(
                NULL, "Arduino�}�C�R���{�[�h���ڑ�����Ă��܂���.",
                "Error", MB_OK | MB_ICONEXCLAMATION);
        else
            ::MessageBoxA(
                NULL, "Arduino�}�C�R���{�[�h�Ɛڑ��ł��܂���ł���.",
                "Error", MB_OK | MB_ICONEXCLAMATION);

        this->mIsConnected = false;
        return;
    }

    // �V���A���|�[�g�̌��݂̃p�����[�^�̎擾
    DCB dcbSerialInputParameters;
    ZeroMemory(&dcbSerialInputParameters, sizeof(DCB));

    if (!::GetCommState(this->mHandle, &dcbSerialInputParameters)) {
        ::MessageBoxA(
            NULL, "�V���A���|�[�g�̌��݂̃p�����[�^�̎擾�Ɏ��s���܂���.",
            "Error", MB_OK | MB_ICONEXCLAMATION);
        this->mIsConnected = false;
        return;
    }

    // �V���A���|�[�g�̃p�����[�^�̐ݒ�
    dcbSerialInputParameters.BaudRate = CBR_57600;
    dcbSerialInputParameters.ByteSize = 8;
    dcbSerialInputParameters.StopBits = ONESTOPBIT;
    dcbSerialInputParameters.Parity = NOPARITY;
    dcbSerialInputParameters.fDtrControl = DTR_CONTROL_ENABLE;

    if (!::SetCommState(this->mHandle, &dcbSerialInputParameters)) {
        ::MessageBoxA(
            NULL, "�V���A���|�[�g�̃p�����[�^�̐ݒ�Ɏ��s���܂���.",
            "Error", MB_OK | MB_ICONEXCLAMATION);
        this->mIsConnected = false;
        return;
    }

    // �V���A���|�[�g�̐ڑ��̊���
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

        // ���s�R�[�h�����o������ǂݍ��݂��I����
        if (pBuffer[i] == '\n')
            return numOfBytesRead;
    }

    return 0;
}

//
// PipeObject�\����
//
struct PipeObject
{
    int mPositionX;             // �y�ǂ̍��[��X���W
    int mTopPipeHeight;         // �㑤�̓y�ǂ̍���
    int mBottomPipeHeight;      // �����̓y�ǂ̍���
    bool mPassed;               // �y�ǂ�ʉ߂������ǂ���
};

//
// GameState�񋓑�
//
enum class GameState
{
    Start,
    Play,
    GameOver
};

//
// CGame�N���X
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
    CArduinoSerialInput* mArduinoInput;     // �V���A���|�[�g�̓���
    char* mSerialInputBuffer;               // ���̓o�b�t�@
    double mInputValue;                     // �Z���T�̓��͒l
    double mOldInputValue;                  // 1�O�̃Z���T�̓��͒l

    GameState mGameState;                   // �Q�[���̏��

    int mImageHandleBackground;             // �w�i�摜�̃n���h��
    int mImageHandleBird[3];                // ���摜�̃n���h��
    int mImageHandleGround;                 // �n�ʉ摜�̃n���h��
    int mImageHandlePipe;                   // �y�ǉ摜�̃n���h��
    int mImageHandleRestart;                // ���X�^�[�g�摜�̃n���h��
    int mImageHandleScore;                  // �X�R�A�\���摜�̃n���h��

    int mImageBackgroundWidth;              // �w�i�摜�̉���
    int mImageBackgroundHeight;             // �w�i�摜�̏c��
    int mImageGroundWidth;                  // �n�ʉ摜�̉���
    int mImageGroundHeight;                 // �n�ʉ摜�̏c��
    int mImageGroundOffset;                 // �n�ʉ摜�̃X�N���[����
    int mNumOfGroundImages;                 // �n�ʉ摜�̕`���

    int mImagePipeWidth;                    // �y�ǉ摜�̉���
    int mImagePipeHeight;                   // �y�ǉ摜�̏c��

    int mImageScoreWidth;                   // �X�R�A�\���摜�̉���
    int mImageScoreHeight;                  // �X�R�A�\���摜�̏c��

    int mBirdWidth;                         // ���摜�̉���
    int mBirdHeight;                        // ���摜�̏c��
    int mBirdPositionX;                     // ���̍���X���W
    int mBirdPositionY;                     // ���̍���Y���W
    int mBirdPositionMaxY;                  // ���̍���Y���W�̍ő�l

    int mGamePlayThresholdPositionY;        // �v���C��ʂ֑J�ڂ��邽�߂ɕK�v�Ȓ��̍���Y���W��臒l
    int mPipeGenerateCounter;               // �y�ǂ̏o���J�E���^
    int mPipeGenerateCounterThreshold;      // �y�ǂ��o�������邽�߂ɕK�v�ȓy�ǂ̃J�E���^�l��臒l
    int mPipeGap;                           // �㉺�̓y�ǂ̊Ԋu
    std::vector<PipeObject> mPipeObjects;   // �y�ǃI�u�W�F�N�g�̃��X�g

    int mFontHandle;                        // �t�H���g�̃n���h��

    int mScore;                             // �X�R�A
    int mBestScore;                         // �x�X�g�X�R�A
    double mRestartThresholdPositionY;      // ���X�^�[�g���邽�߂ɕK�v�ȃZ���T�̓��͒l��臒l

    static const char* ApplicationName;     // �A�v���P�[�V������
    static const int WindowWidth;           // �E�B���h�E�̉���
    static const int WindowHeight;          // �E�B���h�E�̏c��
    static const int ColorBitDepth;         // �J���[�r�b�g��
    static const int RefreshRate;           // �t���[�����[�g
    static const char* PortName;            // �ڑ��|�[�g��
    static const int SerialInputBufferSize; // ���̓o�b�t�@�̃T�C�Y
};

const char* CGame::ApplicationName = "ArduinoGame";     // �A�v���P�[�V������
const int CGame::WindowWidth = 768;                     // �E�B���h�E�̉���
const int CGame::WindowHeight = 1024;                   // �E�B���h�E�̏c��
const int CGame::ColorBitDepth = 32;                    // �J���[�r�b�g��
const int CGame::RefreshRate = 60;                      // �t���[�����[�g
const char* CGame::PortName = "\\\\.\\COM3";            // �ڑ��|�[�g��
const int CGame::SerialInputBufferSize = 256;           // ���̓o�b�t�@�̃T�C�Y

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
    // ���̓o�b�t�@�̊m��
    this->mSerialInputBuffer = new char[CGame::SerialInputBufferSize];

    if (this->mSerialInputBuffer == nullptr)
        return false;

    // �V���A���|�[�g�ڑ��̏�����
    this->mArduinoInput = new CArduinoSerialInput(CGame::PortName);

    if (this->mArduinoInput == nullptr) {
        ::MessageBoxA(
            NULL, "Arduino�}�C�R���{�[�h�Ƃ̐ڑ��Ɏ��s���܂���.",
            CGame::ApplicationName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    if (!this->mArduinoInput->IsConnected()) {
        ::MessageBoxA(
            NULL, "Arduino�}�C�R���{�[�h�Ƃ̐ڑ��Ɏ��s���܂���.",
            CGame::ApplicationName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    ::MessageBoxA(
        NULL, "Arduino�}�C�R���{�[�h�Ƃ̐ڑ����m������܂���.",
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
    // �V���A���|�[�g����f�[�^���擾
    int bytesRead = this->mArduinoInput->Read(
        this->mSerialInputBuffer, CGame::SerialInputBufferSize);
    
    if (!bytesRead)
        return;

    this->mSerialInputBuffer[bytesRead] = '\0';

    // �Z���T�̏o�͒l�̃f�o�b�O�o��
    // ::OutputDebugStringA(static_cast<LPCSTR>(this->mSerialInputBuffer));

    // �Z���T�̏o�͒l�̓ǂݍ���
    int inputValue;
    int inputValueDummy;

    if (sscanf_s(this->mSerialInputBuffer, "%d,%d", &inputValue, &inputValueDummy) != 2)
        return;

    // �w���d�ݕt���ړ����ς̌v�Z
    this->mInputValue = 0.95 * this->mInputValue + 0.05 * static_cast<double>(inputValue);
}

void CGame::LoadImages()
{
    // �摜�̓ǂݍ���
    this->mImageHandleBackground = DxLib::LoadGraph("Images/Background.png");
    this->mImageHandleGround = DxLib::LoadGraph("Images/Ground.png");
    this->mImageHandlePipe = DxLib::LoadGraph("Images/Pipe.png");
    this->mImageHandleRestart = DxLib::LoadGraph("Images/Restart.png");
    this->mImageHandleScore = DxLib::LoadGraph("Images/Score.png");

    // �L�����N�^�[�̉摜�̕����ǂݍ���
    this->mBirdWidth = 92;
    this->mBirdHeight = 64;
    DxLib::LoadDivGraph(
        "Images/Bird.png", 3, 3, 1,
        this->mBirdWidth, this->mBirdHeight, this->mImageHandleBird);

    // �w�i�摜�̃T�C�Y���擾
    DxLib::GetGraphSize(
        this->mImageHandleBackground,
        &this->mImageBackgroundWidth, &this->mImageBackgroundHeight);

    // �n�ʉ摜�̃T�C�Y���擾
    DxLib::GetGraphSize(
        this->mImageHandleGround,
        &this->mImageGroundWidth, &this->mImageGroundHeight);

    // �y�ǉ摜�̃T�C�Y���擾
    DxLib::GetGraphSize(
        this->mImageHandlePipe,
        &this->mImagePipeWidth, &this->mImagePipeHeight);

    // �X�R�A�\���摜�̃T�C�Y���擾
    DxLib::GetGraphSize(
        this->mImageHandleScore,
        &this->mImageScoreWidth, &this->mImageScoreHeight);
}

void CGame::LoadFonts()
{
    // �t�H���g�̏�����
    this->mFontHandle = DxLib::CreateFontToHandle("Consolas", 48, -1, DX_FONTTYPE_ANTIALIASING_4X4);
}

void CGame::InitializeParameters()
{
    // �n�ʉ摜�̌����v�Z
    this->mNumOfGroundImages = this->mImageBackgroundWidth / this->mImageGroundWidth + 2;

    // �L�����N�^�[�̉������̍��W���v�Z
    this->mBirdPositionX =
        static_cast<int>(static_cast<double>(CGame::WindowWidth) * 0.4) - this->mBirdWidth / 2;

    // �L�����N�^�[�̍��W�̍ő�l���v�Z
    this->mBirdPositionMaxY =
        CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight;

    // �v���C��ʂ֑J�ڂ��邽�߂ɕK�v�ȃL�����N�^�[�̍��W��臒l���v�Z
    this->mGamePlayThresholdPositionY =
        CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight - 100;

    // �y�ǂ��o�������邽�߂ɕK�v�ȃJ�E���^��臒l���v�Z
    this->mPipeGenerateCounterThreshold =
        static_cast<int>(static_cast<double>(this->mImagePipeWidth) * 1.2);

    // �y�ǂ̏㉺�̊Ԋu���v�Z
    this->mPipeGap = static_cast<int>(static_cast<double>(CGame::WindowWidth) * 0.3);

    // ���X�^�[�g���邽�߂ɕK�v�ȃZ���T�̓��͒l��臒l�̐ݒ�
    this->mRestartThresholdPositionY = 500.0;
}

void CGame::Update()
{
    // �n�ʂ̈ړ�
    this->mImageGroundOffset = (this->mImageGroundOffset + 4) % this->mImageGroundWidth;

    switch (this->mGameState) {
        case GameState::Start:
        {
            // �L�����N�^�[�̈ʒu�̌v�Z
            this->mBirdPositionY = std::min(
                this->mBirdPositionMaxY,
                CGame::WindowHeight - static_cast<int>(this->mInputValue) - this->mBirdHeight);

            // �L�����N�^�[�̍��W��臒l�����������v���C��ʂ֑J��
            if (this->mBirdPositionY < this->mGamePlayThresholdPositionY)
                this->mGameState = GameState::Play;

            break;
        }

        case GameState::Play:
        {
            // �L�����N�^�[�̈ʒu�̌v�Z
            this->mBirdPositionY = std::min(
                this->mBirdPositionMaxY,
                CGame::WindowHeight - static_cast<int>(this->mInputValue) - this->mBirdHeight);

            // �y�ǂ̈ړ�
            for (auto& pipeObject : this->mPipeObjects)
                pipeObject.mPositionX -= 4;

            // ��ʂ���͂ݏo���y�ǂ�����
            this->mPipeObjects.erase(
                std::remove_if(
                    this->mPipeObjects.begin(),
                    this->mPipeObjects.end(),
                    [this](const PipeObject& pipeObject) {
                        return pipeObject.mPositionX < -this->mImagePipeWidth;
                }),
                this->mPipeObjects.end());

            // �y�ǂ̏o���J�E���^�̍X�V
            this->mPipeGenerateCounter++;

            // �y�ǂ̏o���J�E���^�����l����������o��������
            if (this->mPipeGenerateCounter >= this->mPipeGenerateCounterThreshold) {
                this->mPipeGenerateCounter = 0;

                // �V�����y�ǃI�u�W�F�N�g�̍쐬
                PipeObject pipeObject;
                pipeObject.mPositionX = CGame::WindowWidth + 128;
                pipeObject.mTopPipeHeight =
                    DxLib::GetRand(CGame::WindowHeight - this->mImageGroundHeight - this->mPipeGap);
                pipeObject.mBottomPipeHeight =
                    CGame::WindowHeight - this->mImageGroundHeight -
                    pipeObject.mTopPipeHeight - this->mPipeGap;
                pipeObject.mPassed = false;

                // �V�����y�ǃI�u�W�F�N�g�̒ǉ�
                this->mPipeObjects.push_back(pipeObject);
            }

            // �y�ǂƃL�����N�^�̏Փ˂̌��m
            auto collidePipeIterator = std::find_if(
                this->mPipeObjects.begin(), this->mPipeObjects.end(),
                [this](const PipeObject& pipeObject) {
                    // �y�ǂƂ̏Փ˔���(������)
                    if (this->mBirdPositionX < pipeObject.mPositionX + this->mImagePipeWidth &&
                        this->mBirdPositionX + this->mBirdWidth > pipeObject.mPositionX)
                        // �㑤�̓y�ǂƂ̏Փ˔���(�c����)
                        if (this->mBirdPositionY < pipeObject.mTopPipeHeight &&
                            this->mBirdPositionY + this->mBirdHeight > 0.0)
                            return true;
                        // �����̓y�ǂƂ̏Փ˔���(�c����)
                        else if (this->mBirdPositionY < CGame::WindowHeight - this->mImageGroundHeight &&
                            this->mBirdPositionY + this->mBirdHeight >
                            CGame::WindowHeight - this->mImageGroundHeight - pipeObject.mBottomPipeHeight)
                            return true;

                    return false;
            });
            
            // �Փ˂�����Q�[���I�[�o�[��ʂ֑J��
            if (collidePipeIterator != this->mPipeObjects.end()) {
                this->mGameState = GameState::GameOver;

                // �x�X�g�X�R�A�Ɣ�r���Č��݂̃X�R�A�̕����傫����΍X�V
                this->mBestScore = std::max(this->mBestScore, this->mScore);

                // �Z���T�̓��͒l�̃N���A
                this->mInputValue = 0.0;
                this->mOldInputValue = 0.0;
            }

            // �y�ǂ��L�����N�^���ʂ蔲�������ǂ����𒲂ׂ�, �V���ɒʂ蔲�����y�ǂ�����΃X�R�A�����Z
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
            // �L�����N�^�[�̈ʒu�̌v�Z(�y�ǂɏՓ˂�����ɒn�ʂɗ���)
            this->mBirdPositionY = std::min(
                this->mBirdPositionY + 32,
                CGame::WindowHeight - this->mImageGroundHeight - this->mBirdHeight);

            // �Z���T�̓��͒l��臒l�𒴂����烊�X�^�[�g
            if (this->mInputValue > this->mRestartThresholdPositionY) {
                this->mGameState = GameState::Start;

                // �Z���T�̓��͒l�̃N���A
                this->mInputValue = 0.0;
                this->mOldInputValue = 0.0;

                // �y�ǂ̏o���J�E���^�̃��Z�b�g
                this->mPipeGenerateCounter = 0;

                // �y�ǃI�u�W�F�N�g�̃��X�g�̃N���A
                this->mPipeObjects.clear();

                // �X�R�A�̃��Z�b�g
                this->mScore = 0;
            }
            break;
        }
    }
}

void CGame::Draw()
{
    // �f�o�b�O�o��
    /*
    DxLib::clsDx();
    DxLib::printfDx("%s\n",
        (this->mGameState == GameState::Start) ? "GameState::Start" :
        (this->mGameState == GameState::Play) ? "GameState::Play" :
        (this->mGameState == GameState::GameOver) ? "GameState::GameOver" : "Unknown");
    DxLib::printfDx("%u", this->mPipeObjects.size());
    */

    // �w�i�摜�̕`��
    DxLib::DrawGraph(0, 0, this->mImageHandleBackground, FALSE);

    switch (this->mGameState) {
        case GameState::Start:
        {
            // �L�����N�^�[�̕`��
            DxLib::DrawGraph(
                this->mBirdPositionX,
                this->mBirdPositionY,
                this->mImageHandleBird[0], TRUE);
            break;
        }

        case GameState::Play:
        {
            // �y�ǂ̕`��
            for (const auto& pipeObject : this->mPipeObjects) {
                // �y�ǂ̏㑤�̕���
                DxLib::DrawRotaGraph(
                    pipeObject.mPositionX + this->mImagePipeWidth / 2,
                    pipeObject.mTopPipeHeight - this->mImagePipeHeight / 2,
                    1.0, ConvertDegreeToRadian<double>(180.0),
                    this->mImageHandlePipe, FALSE, FALSE, FALSE);

                // �y�ǂ̉����̕���
                DxLib::DrawGraph(
                    pipeObject.mPositionX,
                    CGame::WindowHeight - this->mImageGroundHeight - pipeObject.mBottomPipeHeight,
                    this->mImageHandlePipe, FALSE);
            }

            // �L�����N�^�[�̕`��
            DxLib::DrawGraph(
                this->mBirdPositionX,
                this->mBirdPositionY,
                this->mImageHandleBird[0], TRUE);

            // �X�R�A�̕`��
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
            // �L�����N�^�[�̕`��
            DxLib::DrawRotaGraph(
                this->mBirdPositionX - this->mBirdWidth / 2,
                this->mBirdPositionY + this->mBirdHeight / 2,
                1.0, ConvertDegreeToRadian<double>(90.0),
                this->mImageHandleBird[0], TRUE, FALSE, FALSE);

            // �X�R�A�̕\���摜�̕`��
            DxLib::DrawGraph(
                (CGame::WindowWidth - this->mImageScoreWidth) / 2,
                static_cast<int>(static_cast<double>(CGame::WindowHeight) * 0.2),
                this->mImageHandleScore, TRUE);

            // �X�R�A�̕`��
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

    // �n�ʉ摜�̕`��
    for (int i = 0; i < this->mNumOfGroundImages; ++i) {
        DxLib::DrawGraph(
            this->mImageGroundWidth * i - this->mImageGroundOffset,
            this->mImageBackgroundHeight,
            this->mImageHandleGround, FALSE);
    }
}

int CGame::Run()
{
    // �V���A���|�[�g�ڑ��̏�����
    this->InitializeArduinoInput();

    // Dx���C�u�����̏�����
    DxLib::ChangeWindowMode(TRUE);
    DxLib::SetGraphMode(
        CGame::WindowWidth, CGame::WindowHeight,
        CGame::ColorBitDepth, CGame::RefreshRate);
    DxLib::SetWindowSize(CGame::WindowWidth, CGame::WindowHeight);
    DxLib::SetMainWindowText(CGame::ApplicationName);
    DxLib::SetOutApplicationLogValidFlag(FALSE);

    if (DxLib::DxLib_Init() == -1)
        return -1;

    // �摜�̓ǂݍ���
    this->LoadImages();

    // �t�H���g�̓ǂݍ���
    this->LoadFonts();

    // �p�����[�^�̏�����
    this->InitializeParameters();

    while (DxLib::ProcessMessage() == 0) {
        DxLib::ClearDrawScreen();
        DxLib::SetDrawScreen(DX_SCREEN_BACK);

        // �Z���T����̓d���l���擾
        this->HandleInput();

        // �X�V����
        this->Update();

        // �`�揈��
        this->Draw();

        DxLib::ScreenFlip();
    }

    // Dx���C�u�����̏I������
    DxLib::DxLib_End();

    // �V���A���|�[�g�ڑ��̏I������
    this->FinalizeArduinoInput();

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return CGame::GetInstance()->Run();
}
