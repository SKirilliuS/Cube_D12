#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h> // Для компиляции шейдеров на лету
#include <wrl.h>
#include <iostream>
#include <DirectXMath.h> // Математика для 3D-трансформаций
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
void RenderScene();                // Запись команд отрисовки и отправка их на GPU

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
    // Описываем прямой корневой дескриптор для константного буфера матриц (CBV)
    D3D12_ROOT_PARAMETER rootParameters[1] = {};

    // Настраиваем как корневой дескриптор, а не таблицу!
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // Тип: Constant Buffer View
    rootParameters[0].Descriptor.ShaderRegister = 0;                 // Регистр b0 в HLSL
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // Виден только вершинному шейдеру

    // Разрешаем входной ассемблер для передачи вершин куба
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1; // 1 параметр
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = rootSignatureFlags;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) std::cout << "Ошибка сериализации Root Sig: " << (char*)errorBlob->GetBufferPointer() << "\n";
        return false;
    }

    hr = g_Device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature));
    if (FAILED(hr)) return false;

    std::cout << "DirectX 12: Корневая сигнатура (прямой CBV) успешно создана.\n";
    return true;
}

bool CreatePipelineStateObject()
{
    // ИСПРАВЛЕНО: Добавлена буква D в название интерфейса ID3DBlob
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
        "vs_5_0",
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
        "ps_5_0",
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

    // ИСПРАВЛЕНО: Альтернативный способ заполнения дефолтных настроек БЕЗ d3dx12.h на случай его отсутствия
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

    // Отключаем буфер глубины для этого этапа
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    psoDesc.SampleDesc.Count = 1;

    hr = g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState));
    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось создать Graphics Pipeline State Object (PSO)!\n";
        return false;
    }

    std::cout << "DirectX 12: Шейдеры скомпилированы, объект PSO успешно собран.\n";
    return true;
}

bool CreateCubeGeometry()
{
    // ============================================================================
    // 1. ОПРЕДЕЛЕНИЕ ДАННЫХ КУБА (Если они еще не объявлены глобально)
    // ============================================================================
    // 8 вершин куба (позиция X,Y,Z и цвет R,G,B,A)
   /* Vertex cubeVertices[] = {
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) }, // 0
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, // 1
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // 2
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) }, // 3
        { XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // 4
        { XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }, // 5
        { XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) }, // 6
        { XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }  // 7
    };
    / Индексы для сборки 12 треугольников (по 2 на каждую из 6 граней куба)
    WORD cubeIndices[] = {
        0,2,1, 1,2,3,  // Левая
        4,5,6, 5,7,6,  // Правая
        0,1,4, 1,5,4,  // Нижняя
        2,6,3, 3,6,7,  // Верхняя
        0,4,2, 4,6,2,  // Передняя
        1,3,5, 3,7,5   // Задняя
    };*/
    // Тестовый треугольник по центру экрана
    Vertex cubeVertices[] = {
        { XMFLOAT3(0.0f,  0.5f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // Верх (Красный)
        { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // Право (Зеленый)
        { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }  // Лево (Синий)
    };

    // Всего один треугольник (3 индекса вместо 36)
    WORD cubeIndices[] = { 0, 1, 2 };
  

    UINT vertexBufferSize = sizeof(cubeVertices);
    UINT indexBufferSize = sizeof(cubeIndices);

    // Вспомогательная структура для свойств кучи (Upload-тип для загрузки с CPU)
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Вспомогательная структура описания буфера
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

    // ============================================================================
    // 2. СОЗДАНИЕ И ЗАПОЛНЕНИЕ ВЕРШИННОГО БУФЕРА (VERTEX BUFFER)
    // ============================================================================
    bufferDesc.Width = vertexBufferSize; // Задаем размер памяти под вершины

    HRESULT hr = g_Device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_VertexBuffer)
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось выделить память под Vertex Buffer!\n";
        return false;
    }

    // Копируем данные из ОЗУ (CPU) в созданный буфер видеокарты (GPU)
    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // Мы не собираемся читать этот буфер на CPU
    hr = g_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
    g_VertexBuffer->Unmap(0, nullptr);

    // Настраиваем View (представление) вершинного буфера для конвейера
    g_VertexBufferView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
    g_VertexBufferView.StrideInBytes = sizeof(Vertex); // Размер одной вершины
    g_VertexBufferView.SizeInBytes = vertexBufferSize;  // Общий размер буфера

    // ============================================================================
    // 3. СОЗДАНИЕ И ЗАПОЛНЕНИЕ ИНДЕКСНОГО БУФЕРА (INDEX BUFFER)
    // ============================================================================
    bufferDesc.Width = indexBufferSize; // Задаем размер памяти под индексы

    hr = g_Device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_IndexBuffer)
    );

    if (FAILED(hr))
    {
        std::cout << "Ошибка: Не удалось выделить память под Index Buffer!\n";
        return false;
    }

    // Копируем индексные данные на GPU
    UINT8* pIndexDataBegin = nullptr;
    hr = g_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
    g_IndexBuffer->Unmap(0, nullptr);

    // Настраиваем View (представление) индексного буфера
    g_IndexBufferView.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
    g_IndexBufferView.Format = DXGI_FORMAT_R16_UINT; // WORD в C++ соответствует 16-битному инту
    g_IndexBufferView.SizeInBytes = indexBufferSize;

    std::cout << "DirectX 12: Геометрия куба (вершины и индексы) успешно загружена на GPU.\n";
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
    // 1. Вращение
    static float angle = 0.0f;
    angle += 0.0005f;
    XMMATRIX rotationX = XMMatrixRotationX(angle);
    XMMATRIX rotationY = XMMatrixRotationY(angle * 0.5f);
    g_WorldMatrix = rotationX * rotationY;

    // 2. Камера (Отодвигаем камеру на -3 по оси Z и направляем ВПЕРЕД на куб в точку 0,0,0)
    XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -3.0f, 0.0f);
    XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // 3. Перспектива (NearZ ОБЯЗАН быть строго положительным, например 0.1f)
    float aspectRatio = static_cast<float>(g_Width) / static_cast<float>(g_Height);
    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);

    // 4. Сборка WVP
    XMMATRIX wvpMatrix = g_WorldMatrix * g_ViewMatrix * g_ProjectionMatrix;
    ConstantBufferWVP cbWvp;
    cbWvp.wvp = XMMatrixTranspose(wvpMatrix);

    memcpy(g_pCbvDataBegin, &cbWvp, sizeof(cbWvp));

}
void RenderScene()
{
    HRESULT hr;

    // ============================================================================
    // 1. ПОДГОТОВКА К ЗАПИСИ КОМАНД
    // ============================================================================
    // Сбрасываем аллокатор и список команд. Это очищает старую память кадра 
    // и открывает список для записи новых команд.
    hr = g_CommandAllocator->Reset();
    if (FAILED(hr)) return;

    // Привязываем список команд к очищенному аллокатору и задаем начальное состояние конвейера (PSO)
    hr = g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
    if (FAILED(hr)) return;

    // ============================================================================
    // 2. НАСТРОЙКА ЭКРАНА И БАРЬЕРОВ РЕСУРСОВ
    // ============================================================================
    // Переводим текущий задний буфер из состояния «Отображение на экране» (PRESENT) 
    // в состояние «Окно отрисовки» (RENDER_TARGET), чтобы видеокарта могла в него писать.
    D3D12_RESOURCE_BARRIER barrierPresentToRender = {};
    barrierPresentToRender.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierPresentToRender.Transition.pResource = g_RenderTargets[g_FrameIndex].Get();
    barrierPresentToRender.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierPresentToRender.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierPresentToRender.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    g_CommandList->ResourceBarrier(1, &barrierPresentToRender);

    // Получаем CPU-адрес дескриптора текущего заднего буфера для очистки и вывода
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += g_FrameIndex * g_RtvDescriptorSize;

    // Настраиваем область просмотра (Viewport) и ножницы (Scissor Rect) на весь размер окна
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(g_Width), static_cast<float>(g_Height), 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, g_Width, g_Height };
    g_CommandList->RSSetViewports(1, &viewport);
    g_CommandList->RSSetScissorRects(1, &scissorRect);

    // Указываем, куда именно мы будем рисовать (наш текущий задний буфер кадра)
    g_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Очищаем экран в красивый темно-синий цвет (RGBA)
    const float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
    g_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // ============================================================================
    // 3. ОТРИСОВКА 3D-КУБА
    // ============================================================================
    // Устанавливаем корневую сигнатуру (контракт)
   // Устанавливаем сигнатуру
    g_CommandList->SetGraphicsRootSignature(g_RootSignature.Get());

    // Прямо передаем GPU адрес константного буфера в параметр 0 (без всяких куч и таблиц дескрипторов)
    g_CommandList->SetGraphicsRootConstantBufferView(0, g_ConstantBuffer->GetGPUVirtualAddress());



    // Говорим видеокарте, как интерпретировать вершины (рисуем треугольниками)
    g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Привязываем буфер вершин и буфер индексов нашего куба
    g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);
    g_CommandList->IASetIndexBuffer(&g_IndexBufferView);

    // Вызываем команду отрисовки куба по индексам. 
    // 36 — это количество индексов куба (6 граней * 2 треугольника * 3 вершины)
    g_CommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // ============================================================================
    // 4. ЗАВЕРШЕНИЕ КАДРА И ОТПРАВКА НА GPU
    // ============================================================================
    // Возвращаем состояние буфера обратно из RENDER_TARGET в PRESENT, чтобы его можно было показать
    D3D12_RESOURCE_BARRIER barrierRenderToPresent = barrierPresentToRender;
    barrierRenderToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierRenderToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    g_CommandList->ResourceBarrier(1, &barrierRenderToPresent);

    // Официально закрываем список команд. Запись завершена!
    hr = g_CommandList->Close();
    if (FAILED(hr)) return;

    // Передаем записанный список команд нашей командной очереди на исполнение в GPU
    ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Меняем передний и задний буферы местами (выводим картинку на экран Windows)
    // Параметр 1 включает вертикальную синхронизацию (V-Sync)
    hr = g_SwapChain->Present(1, 0);
    if (FAILED(hr)) return;

    // Синхронизируем CPU и GPU: ждем, пока видеокарта полностью отрисует этот кадр,
    // перед тем как процессор начнет готовить следующий.
    WaitForGpu();

    // Обновляем индекс буфера для следующего кадра (переключаемся между 0 и 1)
    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();
}
