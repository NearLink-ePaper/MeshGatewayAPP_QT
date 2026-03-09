<?xml version="1.0" encoding = "utf-8" ?>
    <!DOCTYPE TS >
    <TS version="2.1" language = "zh_CN" >
        <context>
        <name>BleManager </name>
        < message > <source>Ready < /source><translation>就绪</translation > </message>
        < message > <source>Scanning... (12s)</source><translation>扫描中... (12秒)</translation > </message>
            < message > <source>Scan finished, found % 1 device(s) < /source><translation>扫描结束，找到 %1 个设备</translation > </message>
                < message > <source>Scanning...found % 1 gateway(s) < /source><translation>扫描中... 找到 %1 个网关</translation > </message>
                    < message > <source>Scan failed: % 1 < /source><translation>扫描失败: %1</translation > </message>
                        < message > <source>Connecting to % 1 ...</source><translation>正在连接 %1 ...</translation > </message>
                            < message > <source>Connected, discovering services...</source><translation>已连接，正在发现服务...</translation > </message>
                                < message > <source>Disconnected < /source><translation>已断开</translation > </message>
                                < message > <source>Connection error: % 1 < /source><translation>连接错误: %1</translation > </message>
                                    < message > <source>Service discovery finished, setting up FFE0...</source><translation>服务发现完成，配置 FFE0...</translation > </message>
                                        < message > <source>FFE0 service not found! < /source><translation>未找到 FFE0 服务！</translation > </message>
                                            < message > <source>FFE1(Notify) characteristic not found! < /source><translation>未找到 FFE1 (通知) 特征！</translation > </message>
                                                < message > <source>FFE2(Write) characteristic not found! < /source><translation>未找到 FFE2 (写入) 特征！</translation > </message>
                                                    < message > <source>Writing CCCD for notifications...</source><translation>写入 CCCD 订阅通知...</translation > </message>
                                                        < message > <source>CCCD not found on FFE1, notifications may not work < /source><translation>FFE1 上未找到 CCCD，通知可能不可用</translation > </message>
                                                            < message > <source>Received: % 1 < /source><translation>收到: %1</translation > </message>
                                                                < message > <source>CCCD subscription successful < /source><translation>CCCD 订阅成功</translation > </message>
                                                                    </context>
                                                                    < context >
                                                                    <name>ScanPage </name>
                                                                    < message > <source>Scan Mesh Gateway < /source><translation>扫描 Mesh 网关</translation > </message>
                                                                        < message > <source>Scanning...</source><translation>扫描中...</translation > </message>
                                                                            < message > <source>Scanning... (% 1s)</source><translation>扫描中... (%1秒)</translation > </message>
                                                                                < message > <source>Click the button above to scan
Auto - discover SLE_GW_XXXX devices </source><translation>点击上方按钮扫描
自动查找 SLE_GW_XXXX 设备 < /translation></message >
    </context>
    < context >
    <name>ConnectedPage </name>
    < message > <source>Disconnect < /source><translation>断开连接</translation > </message>
    < message > <source>Query Nodes < /source><translation>查询节点</translation > </message>
        < message > <source>Enter broadcast data...</source><translation>输入广播数据...</translation > </message>
            < message > <source>Send < /source><translation>发送</translation > </message>
            < message > <source>Communication Log < /source><translation>通信日志</translation > </message>
                < message > <source>Gateway 0x % 1 < /source><translation>网关 0x%1</translation > </message>
                    < message > <source>Topology: Gateway 0x % 1, % 2 node(s) < /source><translation>拓扑: 网关 0x%1, %2 个节点</translation > </message>
                        < message > <source>Query topology < /source><translation>查询拓扑</translation > </message>
                            < message > <source>Broadcast < /source><translation>广播</translation > </message>
                            < message > <source>Clear < /source><translation>清除</translation > </message>
                            < message > <source>Cancel < /source><translation>取消</translation > </message>
                            < message > <source>Multicast Send < /source><translation>组播发送</translation > </message>
                                < message > <source>Multicast: % 1(% 2 / 8) < /source><translation>组播: %1 (%2/8)</translation></message >
                                    <message><source>Send to 0x % 1...< /source><translation>发送到 0x%1...</translation > </message>
                                        < message > <source>Uploading % 1 /% 2 < /source><translation>上传中 %1/ % 2 < /translation></message >
                                        <message><source>Waiting ACK % 1 /% 2 < /source><translation>等待确认 %1/ % 2 < /translation></message >
                                            <message><source>transferring < /source><translation>传输中</translation > </message>
                                            < message > <source>retransmitting < /source><translation>重传中</translation > </message>
                                            < message > <source>Mesh % 1 % 2 /% 3 < /source><translation>网内 %1 %2/ % 3 < /translation></message >
                                            <message><source>Waiting for confirmation...</source><translation>等待确认...</translation > </message>
                                                < message > <source>Cancelled < /source><translation>已取消</translation > </message>
                                                < message > <source>Multicast % 1 /% 2 < /source><translation>组播 %1/ % 2 < /translation></message >
                                                <message><source>Multicast done: % 1 /% 2 OK < /source><translation>组播完成: %1/ % 2 成功 < /translation></message >
                                                    </context>
                                                    < context >
                                                    <name>NodeCardWidget </name>
                                                    < message > <source>Gateway < /source><translation>网关</translation > </message>
                                                    < message > <source>Direct < /source><translation>直连</translation > </message>
                                                    < message > <source>% 1 hop(s) < /source><translation>%1 跳</translation > </message>
                                                        </context>
                                                        < context >
                                                        <name>SendDialog </name>
                                                        < message > <source>Send to 0x % 1 < /source><translation>发送到 0x%1</translation > </message>
                                                            < message > <source>Gateway(local) < /source><translation>网关 (本机)</translation > </message>
                                                            < message > <source>Direct node < /source><translation>直连节点</translation > </message>
                                                                < message > <source>% 1 - hop route < /source><translation>%1 跳路由</translation > </message>
                                                                    < message > <source>Enter data...</source><translation>输入数据...</translation > </message>
                                                                        < message > <source>Cancel < /source><translation>取消</translation > </message>
                                                                        < message > <source>Send < /source><translation>发送</translation > </message>
                                                                        </context>
                                                                        < context >
                                                                        <name>NodeActionDialog </name>
                                                                        < message > <source>Gateway(local) < /source><translation>网关 (本机)</translation > </message>
                                                                        < message > <source>Direct connection < /source><translation>直连节点</translation > </message>
                                                                            < message > <source>% 1 hops routing < /source><translation>%1 跳路由</translation > </message>
                                                                                < message > <source>Last sent image < /source><translation>上次发送的图片</translation > </message>
                                                                                    < message > <source>No send history < /source><translation>暂无发送记录</translation > </message>
                                                                                        < message > <source>Send Text < /source><translation>发送文本</translation > </message>
                                                                                            < message > <source>Send Image < /source><translation>发送图片</translation > </message>
                                                                                                < message > <source>Cancel < /source><translation>取消</translation > </message>
                                                                                                </context>
                                                                                                < context >
                                                                                                <name>CropImageDialog </name>
                                                                                                < message > <source>Crop Image → Multicast % 1 nodes < /source><translation>裁剪图片 → 组播 %1 个节点</translation > </message>
                                                                                                    < message > <source>Crop Image → 0x % 1 < /source><translation>裁剪图片 → 0x%1</translation > </message>
                                                                                                        < message > <source>Drag crop box to adjust area, drag corners to resize < /source><translation>拖动裁剪框调整区域，拖动角落调整大小</translation > </message>
                                                                                                            < message > <source>Ratio < /source><translation>比例</translation > </message>
                                                                                                            < message > <source>Rotate < /source><translation>旋转</translation > </message>
                                                                                                            < message > <source>Cancel < /source><translation>取消</translation > </message>
                                                                                                            < message > <source>Confirm Crop < /source><translation>确认裁剪</translation > </message>
                                                                                                                </context>
                                                                                                                < context >
                                                                                                                <name>ImagePreviewDialog </name>
                                                                                                                < message > <source>Image Preview → Multicast % 1 nodes < /source><translation>图片预览 → 组播 %1 个节点</translation > </message>
                                                                                                                    < message > <source>Image Preview → 0x % 1 < /source><translation>图片预览 → 0x%1</translation > </message>
                                                                                                                        < message > <source>Original < /source><translation>原图</translation > </message>
                                                                                                                        < message > <source>Binary < /source><translation>二值化</translation > </message>
                                                                                                                        < message > <source>Mode < /source><translation>模式</translation > </message>
                                                                                                                        < message > <source>FAST(gateway flow control) < /source><translation>FAST (网关流控)</translation > </message>
                                                                                                                        < message > <source>ACK(per - packet confirm) < /source><translation>ACK (逐包确认)</translation > </message>
                                                                                                                        < message > <source>Cancel < /source><translation>取消</translation > </message>
                                                                                                                        < message > <source>Multicast Send(% 1) < /source><translation>组播发送 (%1)</translation > </message>
                                                                                                                            < message > <source>Send(% 1) < /source><translation>发送 (%1)</translation > </message>
                                                                                                                            </context>
                                                                                                                            < context >
                                                                                                                            <name>MainWindow </name>
                                                                                                                            < message > <source>Connecting...</source><translation>连接中...</translation > </message>
                                                                                                                                < message > <source>Connecting to % 1 ...</source><translation>正在连接 %1 ...</translation > </message>
                                                                                                                                    < message > <source>Images(% 1) < /source><translation>图片 (%1)</translation > </message>
                                                                                                                                    < message > <source>Select Image < /source><translation>选择图片</translation > </message>
                                                                                                                                        < message > <source>→ Multicast image % 1×% 2 to % 3 nodes < /source><translation>→ 组播图片 %1×%2 至 %3 个节点</translation > </message>
                                                                                                                                            < message > <source>→[0x % 1] Image % 2×% 3(% 4 B) < /source><translation>→ [0x%1] 图片 %2×%3 (%4 字节)</translation > </message>
                                                                                                                                                </context>
                                                                                                                                                < context >
                                                                                                                                                <name>QObject </name>
                                                                                                                                                < message > <source>Image received successfully < /source><translation>图片接收成功</translation > </message>
                                                                                                                                                    < message > <source>Target node out of memory < /source><translation>目标节点内存不足</translation > </message>
                                                                                                                                                        < message > <source>Target node receive timeout < /source><translation>目标节点接收超时</translation > </message>
                                                                                                                                                            < message > <source>Target node cancelled < /source><translation>目标节点已取消</translation > </message>
                                                                                                                                                                < message > <source>CRC check failed(packet loss) < /source><translation>CRC 校验失败 (丢包)</translation > </message>
                                                                                                                                                                    < message > <source>Unknown status(% 1) < /source><translation>未知状态 (%1)</translation > </message>
                                                                                                                                                                        </context>
                                                                                                                                                                        </TS>
