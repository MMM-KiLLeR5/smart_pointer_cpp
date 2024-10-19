# Smart Pointers: Custom Implementation (`SharedPtr` and `WeakPtr`)

Этот репозиторий содержит реализацию умных указателей `SharedPtr` и `WeakPtr`. Эти умные указатели позволяют безопасно управлять динамически выделенной памятью и автоматизировать процесс освобождения ресурсов. Реализация основана на использовании счетчиков ссылок и поддерживает шаблонные параметры для универсальности.

## Описание

- **`SharedPtr`** — умный указатель, который управляет динамически выделенным объектом через механизм подсчета ссылок. Объект будет автоматически удален, когда последний экземпляр `SharedPtr` перестанет на него ссылаться.

- **`WeakPtr`** — слабый указатель, который не увеличивает счетчик ссылок на объект, но может получать доступ к объекту, управляемому `SharedPtr`, без предотвращения его удаления. Используется для избежания циклических зависимостей между объектами.

### Основные возможности:

- **Подсчет ссылок**:
    - Управление количеством активных (`shared_count`) и слабых (`weak_count`) ссылок на объект.

- **Безопасное удаление**:
    - Автоматическое освобождение ресурсов, когда объект больше не используется.

- **Шаблонная поддержка**:
    - Универсальность использования с различными типами данных.

- **Создание умных указателей**:
    - Поддержка методов `MakeShared` и `AllocateShared` для безопасного создания `SharedPtr` с использованием стандартных аллокаторов и кастомных методов выделения памяти.

## Особенности

- **`SharedPtr`**:
    - Управляет объектом с подсчетом ссылок.
    - Автоматически удаляет объект, если больше нет активных `SharedPtr`, ссылающихся на этот объект.
    - Поддерживает копирование, перемещение и безопасное управление ресурсами.

- **`WeakPtr`**:
    - Не увеличивает счетчик ссылок на объект.
    - Используется для временного доступа к объекту без продления его времени жизни.
    - Метод `lock()` позволяет безопасно получить `SharedPtr` из `WeakPtr`, если объект еще существует.

- **Создание с кастомными аллокаторами**:
    - Метод `AllocateShared` позволяет создавать `SharedPtr` с пользовательскими аллокаторами для контроля над выделением и управлением памятью.

- **Автоматическое управление памятью**:
    - Система умных указателей эффективно управляет ресурсами, освобождая память, когда объект больше не используется.

