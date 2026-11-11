# Ассеты и лицензии

Все сторонние модели в `Assets/Models`. Перед коммерческим релизом сверить с актуальными страницами авторов.

## Требуют указания автора в титрах

| Что | Папка | Автор | Лицензия | Строка для титров |
|---|---|---|---|---|
| Зомби-женщина (5 анимаций) | `Models/Enemies/ZombieWoman` | heyheythere | CC BY 4.0 | PSX Creatures Free by heyheythere - https://heyheythere.itch.io/psx-creatures-free - CC BY 4.0 |
| Руки с пистолетом и дробовиком от первого лица, пистолет, дробовик, патроны | `Models/Weapons/PSXFirearms` | heyheythere | CC BY 4.0 | PSX Firearms Free by heyheythere - https://heyheythere.itch.io/psx-firearms-free - CC BY 4.0 |
| Старые руки (больше не используются) | `Models/fps_arms.glb` | GDQuest | CC BY-NC-SA 4.0 | **некоммерческая**: удалить из репозитория |

Автор зомби и PSX Firearms указывает, что модели, текстуры и анимации сделаны с помощью ИИ (сгенерированы кодом).

`fp_pistol.glb` и `fp_pump_shotgun.glb` переконвертированы из жёсткой иерархии в скиннинговую скриптом `Tools/rigid_to_skinned.py`, оригиналы в архиве автора.

## CC0 (указывать автора не нужно)

| Что | Папка | Автор | Источник |
|---|---|---|---|
| Universal Animation Library (43 анимации, гуманоидный риг) | `Models/Animations/UAL` | Quaternius | https://quaternius.itch.io/universal-animation-library |
| Glock, Remington 870, охотничья винтовка | `Models/Weapons/PSXGuns` | Amos | https://amos-makes.itch.io/psx-guns-pack |
| Набор выживания (батарейка, фонарь, топор, нож, бинт, фальшфейер, ракетница, патроны, рюкзак, костёр, палатка и др., всё в одном `Survival.fbx`) | `Models/Items/SurvivalPSX` | Elbolilloduro | https://elbolilloduro.itch.io/survival-psx |
| Аптечка и бинт | `Models/Items/Medkit` | Codyanka | https://codyanka.itch.io/tfak |
| Костёр, пожарный топор (текстуры уменьшены до 256²) | `Models/Camp` | 3Dexter3D | https://3dexter.itch.io/demolished-campground-environment |
| Ели, кусты, трава (текстуры «лето» вшиты в glb, остальные сезоны в `textures/`) | `Models/Nature/RetroNature` | Elegant Crow | https://elegantcrow.itch.io/retro-psx-nature-pack |
| 3 низкополигональных дерева | `Models/Nature/LowPolyTrees` | heyitscdr | https://heyitscdr.itch.io/low-poly-trees |
| Камни | `Models/Nature/Rocks` | Elbolilloduro | https://elbolilloduro.itch.io/objetos-low-poly-estilo-psx |
| Машины и колесо | `Models/Vehicles/PSXCars` | GGBotNet | https://ggbot.itch.io/psx-style-cars |

## CC0-модели со сторонними текстурами

| Что | Папка | Автор | Примечание |
|---|---|---|---|
| Заброшенный дом | `Models/Buildings/AbandonedHouse` | Ace Spectre | модели CC0, текстуры с texturer.com (их условия: http://texturer.com/terms/) |
| 3 лесные хижины | `Models/Buildings/Cabins` | Elbolilloduro | модели CC0; доски и крыша — Poly Haven (CC0), текстура двери `doors_wood_doors_0012` похожа на textures.com, где запрещено распространять текстуры отдельно: лучше заменить |
| Ели и кусты RetroNature | `Models/Nature/RetroNature` | Elegant Crow | текстуры с AmbientCG (CC0) и Pixabay (Pixabay License) |

## Свободная лицензия автора (не CC0)

| Что | Папка | Автор | Условия |
|---|---|---|---|
| Коттедж | `Models/Buildings/Cottage` | KaFe-z | бесплатно в личных и коммерческих проектах, можно менять, указание автора приветствуется |
| Канистра | `Models/Items/JerryCan` | DDV23 | можно в коммерческих играх; **нельзя распространять модель отдельно или в составе пака**. Если репозиторий публичный, лучше держать канистру вне его или заменить |
