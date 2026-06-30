#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h> // Для компиляции шейдеров на лету
#include <wrl.h>
#include <iostream>
#include <DirectXMath.h> // Математика для 3D-трансформаций
// Подключаем d3dx12.h напрямую из системных шаблонов Visual Studio 2022
#include "C:\Users\Пользователь\Downloads\LearningDirectX12-0.0.1\LearningDirectX12-0.0.1\Tutorial1\inc\d3dx12.h"

//#include "shaders.hlsl"
// Подключаем необходимые библиотеки через pragma
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// ============================================================================
// 1. КОНСТАНТЫ И НАСТРОЙКИ
// ============================================================================
const int g_FrameCount = 2; // Двойная буферизация (передний и задний буферы)
int g_Width = 1280;
int g_Height = 720;
const wchar_t* g_WindowClassName = L"D12WindowClass";
const wchar_t* g_WindowTitle = L"DirectX 12 Rotating Cube";


// ============================================================================
// 2. СТРУКТУРЫ ДАННЫХ
// ============================================================================

// Структура вершины куба
struct Vertex {
    XMFLOAT3 position; // Позиция (X, Y, Z)
    XMFLOAT4 color;    // Цвет (R, G, B, A)
};

// Структура константного буфера для передачи MVP-матрицы в шейдер
struct ConstantBufferWVP {
    XMMATRIX wvp; // World-View-Projection матрица
};

// ============================================================================
// 3. ГЛОБАЛЬНЫЕ ИНТЕРФЕЙСЫ (КОНТЕКСТ DIRECTX 12)
// ============================================================================
HWND g_hWnd = nullptr;

// Базовые объекты инициализации
ComPtr<IDXGIFactory4>   g_Factory;
ComPtr<ID3D12Device>    g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain3> g_SwapChain;

// Буферы кадра (RTV) и куча описателей для них
ComPtr<ID3D12DescriptorHeap> g_RtvHeap;
ComPtr<ID3D12Resource>       g_RenderTargets[g_FrameCount];
UINT                         g_RtvDescriptorSize = 0;

// Объекты управления командами
ComPtr<ID3D12CommandAllocator>    g_CommandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_CommandList;

// Конвейер отрисовки (Pipeline State Object) и сигнатура
ComPtr<ID3D12RootSignature>      g_RootSignature;
ComPtr<ID3D12PipelineState>      g_PipelineState;

// Геометрия куба (Ресурсы видеопамяти)
ComPtr<ID3D12Resource> g_VertexBuffer;
D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;

ComPtr<ID3D12Resource> g_IndexBuffer;
D3D12_INDEX_BUFFER_VIEW g_IndexBufferView;

// Константный буфер для матриц
ComPtr<ID3D12Resource> g_ConstantBuffer;
UINT8* g_pCbvDataBegin = nullptr; // Указатель для CPU-записи в видеопамять

// Синхронизация между CPU и GPU
ComPtr<ID3D12Fence> g_Fence;
UINT64              g_FenceValue = 0;
HANDLE              g_FenceEvent = nullptr;
UINT                g_FrameIndex = 0;

// Матрицы для 3D сцены
XMMATRIX g_WorldMatrix;
XMMATRIX g_ViewMatrix;
XMMATRIX g_ProjectionMatrix;
ConstantBufferWVP g_CurrentWVP; // Глобальное хранилище текущей матрицы кадра
ComPtr<ID3D12DescriptorHeap> g_DsvHeap;          // Куча описателей для Z-буфера (Depth Stencil View)
ComPtr<ID3D12Resource>       g_DepthStencilBuffer; // Сама текстура глубины на видеокарте
UINT                         g_DsvDescriptorSize = 0;

// ============================================================================
// 4. ПРОТОТИПЫ ВСЕХ ФУНКЦИЙ ПРОЕКТА
// ============================================================================

// --- Системные функции Win32 ---
bool InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// --- Инициализация инфраструктуры DX12 ---
bool InitDirectX12();              // Шаг 1: Создание Device, CommandQueue, SwapChain
bool CreateDescriptorHeaps();      // Шаг 2: Создание куч под дескрипторы RTV
bool CreateRenderTargetViews();    // Шаг 3: Связывание SwapChain с кучей дескрипторов
bool CreateCommandObjects();       // Шаг 4: Создание Allocator и CommandList

// --- Подготовка конвейера графики и шейдеров ---
bool CreateRootSignature();        // Шаг 5: Описание параметров, передаваемых в шейдер
bool CreatePipelineStateObject();  // Шаг 6: Компиляция шейдеров и сборка PSO (состояния конвейера)

// --- Загрузка геометрии и буферов ---
bool CreateCubeGeometry();         // Шаг 7: Выделение видеопамяти под вершины и индексы куба
bool CreateConstantBuffer();       // Шаг 8: Выделение видеопамяти под MVP-матрицу

// --- Синхронизация ---
bool InitSynchronization();        // Шаг 9: Создание Fence (забора) синхронизации
void WaitForGpu();                 // Функция принудительного ожидания завершения кадра на GPU

// --- Игровой цикл ---
void UpdateScene();                // Обновление логики (расчет угла вращения куба)
void RenderScene();  // Запись команд отрисовки и отправка их на GPU

void LogCameraPosition();
bool CreateDepthStencilBuffer();
// ==========================================
// 2. ГЛАВНАЯ ФУНКЦИЯ (ТОЧКА ВХОДА)
// ==========================================
// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ (ТОЧКА ВХОДА И КАРКАС ПРИЛОЖЕНИЯ)
// ============================================================================
int main()
{
   
        // Вместо setlocale используем явную настройку кодировки консоли Windows
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);

        std::cout << "[Инициализация] Старт подготовки проекта DirectX 12 с нуля...\n\n";
        // ... весь остальной код main ...

  

    // Получаем дескриптор текущего запущенного приложения (требуется для Win32 API)
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // 1. Создание стандартного Win32 окна для вывода графики
    if (!InitWindow(hInstance, SW_SHOW))
    {
        std::cout << "Критическая ошибка: Не удалось создать Win32 окно!\n";
        return -1;
    }
    std::cout << "[Успех] Окно Windows успешно создано.\n";

    // 2. Инициализация базовых интерфейсов DirectX 12 (Device, Command Queue, SwapChain)
    if (!InitDirectX12())
    {
        std::cout << "Критическая ошибка: Сбой инициализации базовой инфраструктуры DX12!\n";
        return -1;
    }

    // 3. Создание кучи описателей для буферов отображения (RTV Heap)
    if (!CreateDescriptorHeaps())
    {
        std::cout << "Критическая ошибка: Не удалось выделить память под дескрипторы RTV!\n";
        return -1;
    }

    // 4. Извлечение текстур буферов из SwapChain и их привязка к дескрипторам
    if (!CreateRenderTargetViews())
    {
        std::cout << "Критическая ошибка: Не удалось связать буферы кадра с SwapChain!\n";
        return -1;
    }
  
    if (!CreateDepthStencilBuffer()) {
        std::cout << "Критическая ошибка: Не удалось создать Z-буфер глубины!\n";
        return -1;
    }
    // 5. Создание инструментов записи низкоуровневых графических команд (Allocator и CommandList)
    if (!CreateCommandObjects())
    {
        std::cout << "Критическая ошибка: Ошибка создания структур записи команд!\n";
        return -1;
    }

    // 6. Создание корневой сигнатуры (контракта между C++ кодом и шейдерами)
    if (!CreateRootSignature())
    {
        std::cout << "Критическая ошибка: Не удалось скомпоновать Root Signature!\n";
        return -1;
    }

    // 7. Компиляция файлов HLSL и сборка объекта состояния графического конвейера (PSO)
    if (!CreatePipelineStateObject())
    {
        std::cout << "Критическая ошибка: Ошибка компиляции шейдеров или сборки конвейера PSO!\n";
        return -1;
    }

    // 8. Выделение памяти на GPU и прямая загрузка координат вершин и индексов куба
    if (!CreateCubeGeometry())
    {
        std::cout << "Критическая ошибка: Не удалось загрузить 3D-геометрию куба в видеопамять!\n";
        return -1;
    }

    // 9. Выделение 256 байт памяти на GPU под константный буфер матриц трансформации (CBV)
    if (!CreateConstantBuffer())
    {
        std::cout << "Критическая ошибка: Не удалось подготовить константный буфер под матрицы!\n";
        return -1;
    }

    // 10. Активация инструментов синхронизации (Fence и Win32 Event) между процессором и видеокартой
    if (!InitSynchronization())
    {
        std::cout << "Критическая ошибка: Ошибка запуска системы синхронизации!\n";
        return -1;
    }

    std::cout << "\n======================================================\n";
    std::cout << " ВСЕ СИСТЕМЫ ЗАПУЩЕНЫ! НАЧИНАЕТСЯ ОТРИСОВКА 3D-КУБА \n";
    std::cout << "======================================================\n\n";

    // ============================================================================
    // ГЛАВНЫЙ ИГРОВОЙ ЦИКЛ ПРИЛОЖЕНИЯ
    // ============================================================================
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Проверяем, есть ли системные сообщения от операционной системы Windows
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // Транслируем сообщения виртуальных клавиш
            DispatchMessage(&msg);  // Отправляем сообщение в нашу функцию WindowProc
        }
        else
        {
            // Если у Windows нет срочных задач — всё свободное время отдаем графике
            UpdateScene(); // Шаг А: Считаем время, матрицы и вращаем куб в ОЗУ
            RenderScene(); // Шаг Б: Записываем команды рендеринга и отправляем задачу на GPU
        }
    }

    // ============================================================================
    // БЕЗОПАСНОЕ ОЧИЩЕНИЕ И ВЫХОД
    // ============================================================================
    std::cout << "[Завершение] Получен сигнал WM_QUIT. Корректно закрываем приложение...\n";

    // Принудительно ждем, пока видеокарта завершит выполнение последних команд рендеринга,
    // перед тем как Windows сотрет из памяти дескрипторы и умные указатели ComPtr.
    WaitForGpu();

    // Закрываем системный дескриптор события Windows
    if (g_FenceEvent != nullptr)
    {
        CloseHandle(g_FenceEvent);
    }

    std::cout << "[Выход] Программа успешно завершена.\n";
    return 0;
}

// ==========================================
// 3. РЕАЛИЗАЦИЯ ФУНКЦИЙ
// ==========================================

// Реализация функции создания окна
bool InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Описываем структуру класса окна
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc; // Указываем функцию обработки событий
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = g_WindowClassName;

    // Регистрируем класс в системе
    if (!RegisterClassExW(&wc)) return false;

    // Вычисляем реальный размер окна с учетом рамок (чтобы рабочая область была ровно 1280x720)
    RECT windowRect = { 0, 0, g_Width, g_Height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Создаем экземпляр окна
    g_hWnd = CreateWindowExW(
        0,
        g_WindowClassName,
        g_WindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hWnd) return false;

    // Показываем окно на экране
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return true;
}

// Реализация оконной процедуры (обработка нажатий, закрытия и т.д.)
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0); // Отправляет сообщение WM_QUIT для выхода из цикла main
        return 0;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
bool InitDirectX12()
{
    // ============================================================================
    // 1. ВКЛЮЧЕНИЕ СЛОЯ ОТЛАДКИ (DEBUG LAYER) - Только для Debug-сборки!
    // ============================================================================
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        std::cout << "[D12 Debug] Слой отладки DirectX 12 успешно активирован.\n";
    }
#endif

    // ============================================================================
    // 2. СОЗДАНИЕ ФАБРИКИ DXGI
    // ============================================================================
    // Завод (фабрика) нужен для перечисления видеокарт и создания цепочки буферов.
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&g_Factory));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать DXGI Factory!\n";
        return false;
    }

    // ============================================================================
    // 3. СОЗДАНИЕ ГРАФИЧЕСКОГО УСТРОЙСТВА (DEVICE)
    // ============================================================================
    // Передаем nullptr, чтобы DirectX автоматически выбрал основную видеокарту.
    // D3D_FEATURE_LEVEL_11_0 гарантирует совместимость с большинством современных GPU.
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_Device));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Видеокарта не поддерживает DirectX 12 Feature Level 11.0!\n";
        return false;
    }

    // ============================================================================
    // 4. СОЗДАНИЕ КОМАНДНОЙ ОЧЕРЕДИ (COMMAND QUEUE)
    // ============================================================================
    // Очередь принимает списки команд от CPU и последовательно отправляет их на GPU.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Прямая очередь для рендеринга
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    hr = g_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_CommandQueue));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Command Queue!\n";
        return false;
    }

    // ============================================================================
    // 5. СОЗДАНИЕ ЦЕПОЧКИ БУФЕРОВ (SWAP CHAIN)
    // ============================================================================
    // Описываем параметры вывода изображения на экран
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = g_FrameCount;                 // 2 буфера кадра (двойная буферизация)
    swapChainDesc.Width = g_Width;                           // Ширина области рисования
    swapChainDesc.Height = g_Height;                         // Высота области рисования
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;       // 32-битный RGBA цвет (по 8 бит на канал)
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Буферы будут использоваться для вывода графики
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Режим Flip — самый эффективный в Windows 10/11
    swapChainDesc.SampleDesc.Count = 1;                      // Без сглаживания MSAA на уровне SwapChain
    swapChainDesc.SampleDesc.Quality = 0;

    // Создаем SwapChain, привязывая его к Win32 окну (g_hWnd) и нашей командной очереди
    ComPtr<IDXGISwapChain1> swapChain1;
    hr = g_Factory->CreateSwapChainForHwnd(
        g_CommandQueue.Get(),
        g_hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Swap Chain для окна!\n";
        return false;
    }

    // Приводим интерфейс к версии SwapChain3 (необходима для работы с DirectX 12)
    hr = swapChain1.As(&g_SwapChain);
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось запросить интерфейс IDXGISwapChain3!\n";
        return false;
    }

    // Запоминаем индекс текущего активного заднего буфера
    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();

    std::cout << "DirectX 12: Базовая инфраструктура (Device, Queue, SwapChain) готова!\n";
    return true;
}
bool CreateDescriptorHeaps()
{
    // ============================================================================
    // СОЗДАНИЕ КУЧИ ОПИСАТЕЛЕЙ ДЛЯ БУФЕРОВ ОТОБРАЖЕНИЯ (RTV HEAP)
    // ============================================================================
    // Описываем структуру кучи дескрипторов для Render Target Views (RTV)
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = g_FrameCount;       // Выделяем 2 слота (для переднего и заднего буферов)
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // Тип кучи — Render Target View
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Эти дескрипторы используются только внутри API (не в шейдерах)
    rtvHeapDesc.NodeMask = 0;

    // Создаем кучу в видеопамяти
    HRESULT hr = g_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_RtvHeap));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать RTV Descriptor Heap!\n";
        return false;
    }

    // Запрашиваем у видеокарты точный размер одного дескриптора RTV в байтах.
    // У разных производителей GPU (NVIDIA, AMD, Intel) этот размер может отличаться,
    // поэтому его необходимо запрашивать динамически для корректного сдвига по памяти.
    g_RtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    std::cout << "DirectX 12: Куча описателей RTV успешно выделена в видеопамяти.\n";
    return true;
}
bool CreateRenderTargetViews()
{
    // Получаем дескриптор (указатель) на самое начало нашей кучи RTV в памяти CPU
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_RtvHeap->GetCPUDescriptorHandleForHeapStart();

    // Связываем каждый буфер из SwapChain с соответствующим слотом в куче описателей
    for (UINT i = 0; i < g_FrameCount; i++)
    {
        // 1. Извлекаем i-й буфер (текстуру окна) из SwapChain
        HRESULT hr = g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&g_RenderTargets[i]));
        if (FAILED(hr))
        {
            std::cout << "Ошибка: Не удалось получить буфер " << i << " из Swap Chain!\n";
            return false;
        }

        // 2. Создаем Render Target View для этого буфера.
        // Передаем nullptr в качестве описания (второй параметр), так как 
        // формат текстуры уже жестко задан при создании самого SwapChain.
        g_Device->CreateRenderTargetView(g_RenderTargets[i].Get(), nullptr, rtvHandle);

        // 3. Сдвигаем дескриптор на следующий слот в куче (прибавляем размер одного элемента)
        rtvHandle.ptr += g_RtvDescriptorSize;
    }

    std::cout << "DirectX 12: Swap Chain успешно связан с кучей описателей RTV (созданы Render Target Views).\n";
    return true;
}
bool CreateCommandObjects()
{
    // ============================================================================
    // 1. СОЗДАНИЕ АЛЛОКАТОРА КОМАНД (COMMAND ALLOCATOR)
    // ============================================================================
    // Аллокатор — это фактическое хранилище (кусок видеопамяти), куда физически 
    // записываются низкоуровневые команды для GPU.
    HRESULT hr = g_Device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&g_CommandAllocator)
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Command Allocator!\n";
        return false;
    }

    // ============================================================================
    // 2. СОЗДАНИЕ СПИСКА КОМАНД (COMMAND LIST)
    // ============================================================================
    // Список команд — это интерфейс (ручка записи), через который мы отдаем приказы:
    // "очисти экран", "установи шейдер", "нарисуй куб". 
    // Четвертый параметр (PipelineState) передаем как nullptr, так как мы настроим 
    // конвейер и шейдеры чуть позже в отдельной функции.
    hr = g_Device->CreateCommandList(
        0,                                 // Маска узла (0 для систем с одной видеокартой)
        D3D12_COMMAND_LIST_TYPE_DIRECT,    // Прямой тип списка команд (для рендеринга)
        g_CommandAllocator.Get(),          // Связываем список с созданным выше аллокатором
        nullptr,                           // Исходное состояние конвейера (PSO)
        IID_PPV_ARGS(&g_CommandList)
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Command List!\n";
        return false;
    }

    // При создании список команд по умолчанию открыт для записи. 
    // Но так как на этапе инициализации мы пока ничего рисовать не собираемся, 
    // его необходимо сразу закрыть, иначе при первой попытке сброса будет ошибка.
    hr = g_CommandList->Close();
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось закрыть Command List после создания!\n";
        return false;
    }

    std::cout << "DirectX 12: Инструменты записи команд (Allocator и CommandList) готовы.\n";
    return true;
}
bool CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters = {};

    // ИСПРАВЛЕНО: Меняем тип параметра с CBV на 32-битные константы
    rootParameters.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters.Constants.ShaderRegister = 0; // Регистр b0 в HLSL
    rootParameters.Constants.RegisterSpace = 0;
    rootParameters.Constants.Num32BitValues = 16; // Передаем ровно 16 float-чисел (матрица 4x4)
    rootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParameters; // Передаем адрес нашего параметра
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = rootSignatureFlags;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) return false;

    hr = g_Device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature));
    if (FAILED(hr)) return false;

    std::cout << "DirectX 12: Корневая сигнатура переведена на 32-битные константы.\n";
    return true;
}

bool CreatePipelineStateObject()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // ============================================================================
    // 1. КОМПИЛЯЦИЯ ВЕРШИННОГО ШЕЙДЕРА
    // ============================================================================
    HRESULT hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr, nullptr,
        "VSMain",
        "vs_5_1",
        compileFlags, 0,
        &vertexShaderBlob, &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob) {
            std::cout << "Ошибка компиляции VS: " << (char*)errorBlob->GetBufferPointer() << "\n";
        }
        return false;
    }

    // ============================================================================
    // 2. КОМПИЛЯЦИЯ ПИКСЕЛЬНОГО ШЕЙДЕРА
    // ============================================================================
    hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr, nullptr,
        "PSMain",
        "ps_5_1",
        compileFlags, 0,
        &pixelShaderBlob, &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob) {
            std::cout << "Ошибка компиляции PS: " << (char*)errorBlob->GetBufferPointer() << "\n";
        }
        return false;
    }

    // ============================================================================
    // 3. ОПИСАНИЕ ФОРМАТА ВЕРШИН (INPUT LAYOUT)
    // ============================================================================
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ============================================================================
    // 4. СБОРКА ОБЪЕКТА СОСТОЯНИЯ КОНВЕЙЕРА (PSO)
    // ============================================================================
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_RootSignature.Get();

    psoDesc.VS = D3D12_SHADER_BYTECODE{ vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = D3D12_SHADER_BYTECODE{ pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

    // Настройка растеризатора
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // Включено: Базовая настройка буфера глубины
    psoDesc.DepthStencilState.DepthEnable = TRUE; // Разрешаем тест глубины
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // Разрешаем видеокарте записывать глубину вершин
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // Рисуем только те пиксели, которые ближе к камере
    psoDesc.DepthStencilState.StencilEnable = FALSE; // Трафарет нам не нужен

    // Настройка смешивания цветов (Blend State)
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // Удалено: Раньше здесь стоял ошибочный блок с отключением DepthEnable = FALSE, мы его полностью стёрли!

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Включено: Передаем формат созданного Z-буфера конвейеру
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    psoDesc.SampleDesc.Count = 1;

    hr = g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Graphics Pipeline State Object (PSO)!\n";
        return false;
    }

    std::cout << "DirectX 12: Шейдеры скомпилированы, объект PSO успешно собран с поддержкой Z-буфера.\n";
    return true;
}

bool CreateCubeGeometry()
{
    Vertex cubeVertices[] = {
        //   Координаты XYZ          |      Цвет RGBA (Яркий Красно-Зеленый куб)
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // Красный
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // Зеленый
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // Красный
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // Зеленый
        { XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // ...
        { XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }
    };


    WORD cubeIndices[] = {
        0,2,1, 1,2,3,  // Левая грань
        4,5,6, 5,7,6,  // Правая грань
        0,1,4, 1,5,4,  // Нижняя грань
        2,6,3, 3,6,7,  // Верхняя грань
        0,4,2, 4,6,2,  // Передняя грань
        1,3,5, 3,7,5   // Задняя грань
    };

    UINT vertexBufferSize = sizeof(cubeVertices);
    UINT indexBufferSize = sizeof(cubeIndices);

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // 1. СОЗДАНИЕ VERTEX BUFFER
    bufferDesc.Width = vertexBufferSize;
    // ИСПРАВЛЕНО: Состояние ресурса изменено на VERTEX_AND_CONSTANT_BUFFER
    HRESULT hr = g_Device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&g_VertexBuffer));
    if (FAILED(hr)) return false;

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    g_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
    g_VertexBuffer->Unmap(0, nullptr);

    g_VertexBufferView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
    g_VertexBufferView.StrideInBytes = sizeof(Vertex); // Шаг одной вершины (28 байт)
    g_VertexBufferView.SizeInBytes = vertexBufferSize;

    // 2. СОЗДАНИЕ INDEX BUFFER
    bufferDesc.Width = indexBufferSize;
    // ИСПРАВЛЕНО: Состояние ресурса изменено на INDEX_BUFFER
    hr = g_Device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_INDEX_BUFFER, nullptr, IID_PPV_ARGS(&g_IndexBuffer));
    if (FAILED(hr)) return false;

    UINT8* pIndexDataBegin = nullptr;
    g_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
    g_IndexBuffer->Unmap(0, nullptr);

    g_IndexBufferView.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
    g_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    g_IndexBufferView.SizeInBytes = indexBufferSize;

    std::cout << "DirectX 12: Геометрия куба успешно инициализирована с правильными стейтами буферов.\n";
    return true;
}

bool CreateConstantBuffer()
{
    // Размер константного буфера должен быть кратен 256 байтам!
    // sizeof(ConstantBufferWVP) равен 64 байтам (матрица 4х4 из float), 
    // поэтому мы выравниваем его до 256 байт.
    UINT constantBufferSize = (sizeof(ConstantBufferWVP) + 255) & ~255;

    // Свойства кучи (Upload-тип, чтобы CPU мог каждый кадр перезаписывать матрицы)
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Описание ресурса (Буфер)
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = constantBufferSize; // Выделяем ровно 256 байт
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Создаем ресурс константного буфера в видеопамяти
    HRESULT hr = g_Device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_ConstantBuffer)
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Constant Buffer!\n";
        return false;
    }

    // Проецируем (Map) память буфера один раз при инициализации.
    // Мы не будем вызывать Unmap до самого закрытия программы. 
    // Это называется Persistent Mapping (постоянное отображение) — 
    // самый быстрый способ обновлять данные каждый кадр.
    D3D12_RANGE readRange = { 0, 0 }; // Чтение с CPU не планируется
    hr = g_ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pCbvDataBegin));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось выполнить Map для Constant Buffer!\n";
        return false;
    }

    std::cout << "DirectX 12: Константный буфер матриц (256 байт) успешно выделен.\n";
    return true;
}
bool InitSynchronization()
{
    // ============================================================================
    // 1. СОЗДАНИЕ ОБЪЕКТА СИНХРОНИЗАЦИИ (FENCE)
    // ============================================================================
    // Fence (забор) — это счетчик, который живет на видеокарте и увеличивается,
    // когда GPU доходит до определенной точки в очереди команд.
    g_FenceValue = 1; // Стартовое значение счетчика

    HRESULT hr = g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Fence объект синхронизации!\n";
        return false;
    }

    // ============================================================================
    // 2. СОЗДАНИЕ СОБЫТИЯ WINDOWS (EVENT)
    // ============================================================================
    // Дескриптор события нужен для того, чтобы заставить процессор (CPU) «уснуть» 
    // и дождаться сигнала от видеокарты (GPU), если она отстает по кадрам.
    g_FenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (g_FenceEvent == nullptr)
    {
        std::cout << "Ошибка: Не удалось создать Win32 Event объект!\n";
        return false;
    }

    std::cout << "DirectX 12: Система синхронизации CPU и GPU успешно запущена.\n";
    return true;
}

// Принудительное ожидание завершения всех задач на GPU
void WaitForGpu()
{
    // 1. Запоминаем текущее значение забора, которое мы хотим ожидать
    const UINT64 fenceValueToWait = g_FenceValue;

    // 2. Добавляем в очередь команд приказ для GPU: 
    // "Когда дойдешь до этого места, обнови значение Fence на видеокарте до fenceValueToWait"
    HRESULT hr = g_CommandQueue->Signal(g_Fence.Get(), fenceValueToWait);
    if (FAILED(hr)) return;

    // 3. Увеличиваем локальный счетчик для следующего кадра
    g_FenceValue++;

    // 4. Проверяем: успела ли видеокарта уже выполнить команду и обновить Fence?
    // Если текущее значение на GPU меньше, чем то, которое мы ждем — значит GPU еще работает.
    if (g_Fence->GetCompletedValue() < fenceValueToWait)
    {
        // Привязываем наше Win32 событие к достижению нужного значения на Fence
        hr = g_Fence->SetEventOnCompletion(fenceValueToWait, g_FenceEvent);
        if (FAILED(hr)) return;

        // Полностью останавливаем поток процессора (CPU) до тех пор, пока GPU не поднимет событие
        WaitForSingleObjectEx(g_FenceEvent, INFINITE, FALSE);
    }
}
void UpdateScene()
{
    // 1. Считаем вращение
    static float angle = 0.0f;
    angle += 0.003f;
    XMMATRIX rotationX = XMMatrixRotationX(angle);
    XMMATRIX rotationY = XMMatrixRotationY(angle * 0.5f);
    XMMATRIX rotationMatrix = rotationX * rotationY;

    // 2. УВЕЛИЧИВАЕМ КУБ (Масштаб)
    XMMATRIX scaleMatrix = XMMatrixScaling(1.5f, 1.5f, 1.5f);

    // 3. СДВИГАЕМ КУБ ПО ОСИ Z ВГЛУБЬ ЭКРАНА
    // Первые два нуля — это X и Y (куб остается строго по центру экрана)
    // Третье число (5.0f) — это сдвиг по оси Z. Куб улетает на 5 единиц вперед от центра мира.
    XMMATRIX translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 5.0f);

    // Объединяем строго в порядке: Масштаб * Вращение * Смещение
    g_WorldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

    // 4. ПОЗИЦИЯ КАМЕРЫ
    // Ставим камеру в абсолютный ноль мира (0,0,0) и направляем строго вперед по оси Z
    XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // Смотрим вперед по оси Z (+1)
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // 5. ПЕРСПЕКТИВА (NearZ = 0.1f, FarZ = 100.0f)
    float aspectRatio = static_cast<float>(g_Width) / static_cast<float>(g_Height);
    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);

    // 6. СБОРКА И ТРАНСПОНИРОВАНИЕ WVP МАТРИЦЫ
    XMMATRIX wvpMatrix = g_WorldMatrix * g_ViewMatrix * g_ProjectionMatrix;
    g_CurrentWVP.wvp = XMMatrixTranspose(wvpMatrix);
}


// ============================================================================
// ОТРЕСОВКА КАДРА (ЗАПИСЬ КОМАНД И ОТПРАВКА НА GPU)
// ============================================================================
void RenderScene()
{
    // 1. Сброс аллокатора и списка команд для нового кадра
    g_CommandAllocator->Reset();
    g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());

    // 2. Барьер памяти: Переводим буфер кадра из состояния PRESENT в RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = g_RenderTargets[g_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    g_CommandList->ResourceBarrier(1, &barrier);

    // 3. Вычисляем CPU-адрес дескриптора для текущего буфера цвета кадра (RTV)
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        g_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_FrameIndex,
        g_RtvDescriptorSize
    );

    // Получаем CPU-адрес дескриптора Z-буфера
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(
        g_DsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    // Жестко настраиваем Viewport и Scissor Rect на размер 1280x720
    CD3DX12_VIEWPORT viewport(0.0f, 0.0f, 1280.0f, 720.0f);
    CD3DX12_RECT scissorRect(0, 0, 1280, 720);
    g_CommandList->RSSetViewports(1, &viewport);
    g_CommandList->RSSetScissorRects(1, &scissorRect);

    // Привязываем к конвейеру одновременно буфер цвета и буфер глубины
    g_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Очищаем экран в красивый темно-синий цвет
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Очищаем Z-буфер в максимальную даль (1.0f)
    g_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Вызываем функцию логирования координат камеры в консоль
    LogCameraPosition();

    // ============================================================================
    // [ВСТАВЛЕНО] ДИАГНОСТИЧЕСКИЙ ЛОГ ОТПРАВЛЯЕМОЙ МАТРИЦЫ WVP НА GPU
    // ============================================================================
    static int matrixLogTimer = 0;
    matrixLogTimer++;
    if (matrixLogTimer % 300 == 0)
    {
        // Превращаем структуру в массив из 16 дробных чисел для чтения
        float* m = reinterpret_cast<float*>(&g_CurrentWVP.wvp);
        std::cout << "\n[GPU Отправка] Матрица WVP, уходящая в SetGraphicsRoot32BitConstants:\n";
        std::cout << "  [" << m[0] << ", " << m[1] << ", " << m[2] << ", " << m[3] << "]\n";
        std::cout << "  [" << m[4] << ", " << m[5] << ", " << m[6] << ", " << m[7] << "]\n";
        std::cout << "  [" << m[8] << ", " << m[9] << ", " << m[10] << ", " << m[11] << "]\n";
        std::cout << "  [" << m[12] << ", " << m[13] << ", " << m[14] << ", " << m[15] << "]\n";

        if (m[0] == 0.0f && m[5] == 0.0f && m[10] == 0.0f) {
            std::cout << "  -> ПРЕДУПРЕЖДЕНИЕ: Матрица занулена или пуста!\n\n";
        }
        else {
            std::cout << "  -> Статус: Матрица заполнена данными, передача идет.\n\n";
        }
    }
    // ============================================================================

    // 4. Настройка шейдерных параметров и отрисовка куба
    g_CommandList->SetGraphicsRootSignature(g_RootSignature.Get());

    // Передаем 16 float-чисел нашей WVP матрицы напрямую в регистры GPU (в b0)
    g_CommandList->SetGraphicsRoot32BitConstants(0, 16, &g_CurrentWVP, 0);

    // Указываем тип примитивов и привязываем буферы куба
    g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);
    g_CommandList->IASetIndexBuffer(&g_IndexBufferView);

    // Даем видеокарте команду отрисовать 36 индексов куба
    g_CommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // 5. Барьер памяти: Переводим буфер обратно в состояние PRESENT для монитора
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_CommandList->ResourceBarrier(1, &barrier);

    // Официально закрываем список команд и отправляем его в командную очередь видеокарты
    g_CommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Выводим готовый кадр на экран Windows
    g_SwapChain->Present(1, 0);

    // Ждем процессором, пока видеокарта физически дорисует этот кадр
    WaitForGpu();

    // Обновляем индекс заднего буфера для следующего шага цикла
    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();
}


bool CreateDepthStencilBuffer()
{
    // 1. Создаем кучу описателей для глубин (DSV Heap) на 1 элемент
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // Тип: Depth Stencil View
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = g_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_DsvHeap));
    if (FAILED(hr)) return false;

    g_DsvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // 2. Описываем саму текстуру Z-буфера (должна идеально совпадать с размером окна)
    D3D12_RESOURCE_DESC depthResourceDesc = {};
    depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthResourceDesc.Alignment = 0;
    depthResourceDesc.Width = g_Width;   // 1280
    depthResourceDesc.Height = g_Height; // 720
    depthResourceDesc.DepthOrArraySize = 1;
    depthResourceDesc.MipLevels = 1;
    depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32 бита на глубину каждого пикселя
    depthResourceDesc.SampleDesc.Count = 1;
    depthResourceDesc.SampleDesc.Quality = 0;
    depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // Флаг: Будет Z-буфером

    // Задаем значение, которым видеокарта будет очищать экран перед каждым кадром (1.0f — максимальная даль)
    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    // Свойства кучи памяти на GPU (Default тип — самая быстрая память видеокарты)
    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Создаем физическую текстуру глубины на видеокарте
    hr = g_Device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthResourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // Изначально открыта для записи глубины
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&g_DepthStencilBuffer)
    );
    if (FAILED(hr)) return false;

    // 3. Создаем ярлык (View) внутри кучи описателей
    g_Device->CreateDepthStencilView(g_DepthStencilBuffer.Get(), nullptr, g_DsvHeap->GetCPUDescriptorHandleForHeapStart());

    std::cout << "DirectX 12: Z-буфер глубины (32-bit Float) успешно выделен на GPU.\n";
    return true;
}
void LogCameraPosition()
{
    static int logTimer = 0;
    logTimer++;
    if (logTimer % 300 == 0) // Выводим координаты раз в 300 кадров
    {
        // Извлекаем позицию камеры из Видовой матрицы (View Matrix) обратно в мировые координаты
        XMMATRIX invView = XMMatrixInverse(nullptr, g_ViewMatrix);
        XMVECTOR camPos = invView.r[3]; // Четвертая строка инвертированной матрицы — это позиция XYZ

        float x = XMVectorGetX(camPos);
        float y = XMVectorGetY(camPos);
        float z = XMVectorGetZ(camPos);

        std::cout << "[Камера] Текущие координаты в 3D мире: X=" << x << ", Y=" << y << ", Z=" << z << "\n";
    }
}
