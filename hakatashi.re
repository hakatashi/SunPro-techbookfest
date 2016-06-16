= インターネットの1秒がもし1年だったら(仮)

//raw[|latex|\\chapterauthor{hakatashi・Mine}]

//lead{

//}

== はじめに

たぶんはじめまして。博多市(@hakatashi)です。
今回は技術書典向けの小企画として、
「インターネットの1秒がもし1年だったら」という
出落ち極まりない記事を書こうと思います。

インターネットというのは光の速さを身をもって感じることができるメディアです。
先日PHPのパッケージマネージャーであるcomposerを高速化するという内容のスライド@<fn>{composer}を
拝見したのですが、そこではcomposerが遅い原因として「光の速さが遅すぎる」というものが挙げられていました。

ユニークな考え方ですが言われてればたしかに道理で、日本からアメリカのサーバーまでハンドシェイクで
何度も往復していると、確かに光といえど100ミリ秒単位で時間を浪費しています。
サーバーが近ければ通信は早いというのは誰もが知っていることですが、
光の速度のせいだと言われるとなにやら圧倒されるものがあります。

この記事では、そんなネットワークの微視的な時間スケールについて徹底的に解剖します。
一回の通信を解析し、それぞれ時系列順に細かく分解し、それぞれの操作でどれくらいの時間が費やされ、
どんなイベントがいつ発生するのかを逐一追っていきたいと思います。

とはいえ、ネットワーク通信におけるタイムスパンはミリ秒単位で数えられます。
今回は、そんな微細な時間の移り変わりをなるべくわかりやすくするため、
@<strong>{通信上の1秒という時間を1年にまで引き伸ばし}、@<strong>{約3000万倍}の時間スケールで
ネットワークのイベントを追跡してみます。

//footnote[composer][@<href>{http://www.slideshare.net/hinakano/composer-phpstudy}]

=== 対象読者

 * ほげ
 * ほげ

=== 共著者について

=== シチュエーション

では、実際にどんな通信を解析するとよいでしょう。
徹底解剖する通信として、なるべく我々が普段慣れ親しんでいて、
それでいてそれなりに複雑で解剖しごたえのある通信を採用したいところです。

そこで、今回は@<strong>{HTTPSプロトコル}の通信をメインに解析しました。
ふだん我々がブラウザから毎日使っているプロトコルなのは言うまでもなく、
SSL/TLSの処理を挟むので、それなりに面白い結果になるのではないでしょうか。

また、HTTPS通信を行う際に必要となるARPやDNS通信についても述べていきます。

さらに、通信を行うサーバーとクライアントの場所ですが、
今回の技術書典の会場でもある東京のインターネット環境から、
さくらインターネットが誇る、石狩データセンターまで通信を行います。

Googleマップによると、東京から、石狩データセンターがある石狩市新港中央までは1177.3km。
光ファイバーに用いられる石英ガラスの屈折率は1.50程度@<fn>{silicon}なので、光は約20万km/sでこの距離を移動します。
つまり、東京から石狩までの物理的な片道時間は約5.9ミリ秒となります。
瞬く間もないほどの時間ですが、時間スケールを1年に引き伸ばすと2日と3時間半もかかります。
往復で4日と7時間。石狩までぶらり旅といった感じですね。

//footnote[silicon][光学ガラス材料 - シグマ光機株式会社 @<href>{http://www.products-sigmakoki.com/category/opt_d/opt_d01.html}]

==== 1秒→1年

1秒が1年になった世界の様子をもう少し見てみましょう。

真空中の光は時速34kmで移動します。
先ほど出てきた光ファイバー中の光速は時速23km程度になるので、
自転車か、休憩しながらのドライブ旅程度の速さになります。
ちなみに男子マラソンの世界記録は時速20.6kmです。@<fn>{marathon}@<fn>{relativity}

3GHzのCPUは、こんな世界でも1秒間に95回という高橋名人の6倍のクロックを刻みます。
PC3-12800のメモリーの転送速度は、
もはやダイアルアップ接続よりも遥かにナローバンドですが、
最大で1秒間に3200ビットの情報を読み出すことができます。

//footnote[marathon][2014年ベルリンマラソンのデニス・キメット選手の記録、2時間2分57秒より。]
//footnote[relativity][ただし、この速度で走行するとこの世界では体重が26%増えるのでオススメしない。]

==== 制約など

今回パケットを解析する上で、話を簡便化するためにいくつかの制約を加えています。

 * DNSのシステムを明確に示すため、ローカルホストにDNSサーバーを置き、
   そこからリクエストのたびにルートDNSサーバーからアドレスを引いています。
 * 時間がかかりすぎるため、DNSSECの検証は省略しています。
 * SSL/TLSの証明書検証は省略しています。

=== 測定環境

測定には、ラップトップマシン上のUbuntuの仮想環境を使用しました。
一回の測定ごとにDNSキャッシュとARPキャッシュをクリアし、
なるべくクリーンな状態で測定を行いました。

また、先程も述べたとおりDNSサーバーはローカルホストに設置し、
そこから外部に向けてアドレスを引くようにしました。

...

== ルーターのアドレス解決

=== 1月1日 午前0時0分 ARPリクエスト送信

あけましておめでとうございます。NHKの「ゆく年くる年」を見ながらインターネットの年が明けます。

この瞬間、東京でまったりと紅白歌合戦を見ていたクライアントちゃんは、
石狩にいるサーバーちゃんに聞きたいことがあるのを思い出しました。
長い長い通信の始まりです。

さっそくクライアントちゃんはサーバーちゃんに手紙を書くことにしました。

クライアントちゃんは何もわかりません。
郵便ポスト(ルーター)の場所も、サーバーちゃんの住所(IPアドレス)も、
いつも使っていた住所録(DNSサーバー)の場所も覚えていません。
キャッシュを削除したクライアントちゃんは記憶を失ってしまったのです。

覚えているのは、ただ石狩にいるはずのサーバーちゃんのことだけ……。

困ったクライアントちゃんは、まずは郵便ポスト(ルーター)を探すことにしました。
家の中や向こう三軒両隣のことならともかく、遠く石狩に住むサーバーちゃんに手紙を届けるには、
何よりポストがなければ話が始まりません。

クライアントちゃんはもうネットワークに接続しているので、
ルーターのIPアドレスはすでに分かっています。
しかし実際にルーターと通信するには、ルーターのMACアドレスが必要です。
これを引くために、@<kw>{ARP, Address Resolution Protocol}と呼ばれる
プロトコルを用います。

ARPリクエストはIPアドレスに対応するMACアドレスを検索するリクエストです。
MACアドレスの@<kw>{プロードキャストアドレス}(FF:FF:FF:FF:FF:FF)に向けて発信することで、
同一ネットワーク内のすべての機器にこのリクエストを送信することができます。

クライアントちゃんは街中に響く声でポストの場所を尋ねました。

=== 1月1日 午前3時26分 ARPレスポンス受信

いったい今何時だと思っているんでしょうか。
クライアントちゃんの声は街中に響き渡り、近隣住民からたいへん大目玉を食らいましたが、
努力の甲斐あって、3時間半後に親切な人がポストの場所を教えてくれました。

ARPは、ブロードキャストでネットワーク全体に配信されたイーサネットフレームに対して、
尋ねられているIPアドレスが自分のものであればMACアドレスを応答するという
シンプルな仕組みで動作します。

今回は事前にARPキャッシュを削除してから通信を行ったので、
通信前に必ずARPの問い合わせが走るようにして計測したのですが、
そもそもARPは頻繁にキャッシュを消去します。@<fn>{arpcache}
クライアントちゃんはとても忘れっぽいのです。

//footnote[arpcache][Ubuntuの場合、キャッシュの有効時間は15分程度。]

== DNS解決

=== 1月1日 午前5時7分 DNSクエリ送信(1回目)

親切なおじさんのおかげで、クライアントちゃんは郵便ポストの場所を知ることができました。
またすぐ忘れてしまうのですが、この世界では約900年後のことなので、特に気にすることはありません。

となると次にクライアントちゃんが知るべきはサーバーちゃんの住所(=IPアドレス)です。
郵便ポストを見つけても相手の住所がわからないと手紙は送れません。
サーバーちゃんの住所を知るために、クライアントちゃんは
ルートサーバーに住所を問い合わせることにしました。

電話番号に電話番号問い合わせサービスがあるように、ネットワークの世界にも、
ドメイン名から相手のアドレスを問い合わせるための@<kw>{DNS, Domain Name Service}があります。

電話番号の場合は104番という番号を知っていれば他の番号を問い合わせることができます。
インターネットにおける“104”は、世界に13個存在する@<kw>{ルートサーバー}のアドレスです。
今回は、13個の中で唯一日本の団体が管理している、Mルートサーバーに問い合わせるように
ルートサーバーの設定を変更しています。@<fn>{dnscluster}

サーバーちゃんの住所を一秒でも早く正確に知りたいクライアントちゃんは、
世界中に数あるDNSサーバーの中でも最も権威あるルートサーバーに一筆したためることにしました。
あらゆるキャッシュを削除したクライアントちゃんも、一番大事なルートサーバーの住所は覚えています。
あれこれ悩んで手紙を書き、ようやく完成したのはそれから1時間半後のことでした。
早朝の冷たい冬風が骨身に染みます。クライアントちゃんはコートを厚めに羽織って
ポストに手紙を投函しに行きました。

一刻も早く返事が帰ってくることを願って……。

//footnote[dnscluster][もっとも、ルートサーバーはクラスタ構成になっており国単位で分散しているため、必ずしも問い合わせたマシンが日本に存在するかどうかは保証されないのですが……。]

=== 1月14日 午前3時57分 DNSレスポンス受信(1回目)

最初に手紙を送ってから2週間が経過しました。
冬休みも終わって正月気分もようやく抜けてくる頃です。
もうこの時点で直接会いに行ったほうが手っ取り早いんじゃないかという気がしますが、
石狩は遠いのです。そんな気軽に会いに行けないのです。たぶん。

ともあれ、ルートサーバーからようやく返事が帰ってきました。
郵便局が怠慢なのかルートサーバーがお役所仕事をしてるのか知りませんが、
とにかくこれでサーバーちゃんの住所を知ることができた……というわけではありません。

ルートサーバーは世界中のすべてのマシンのIPアドレスを記録しているわけではありません。
ドメイン名を問い合わせられたDNSサーバーは、自らが委任するDNSゾーンのネームサーバーの情報を返します。
今回問い合わせたドメイン名はさくらのVPSサーバーにデフォルトで割り当てられたドメイン@<fn>{sakuradomain}なので、
ルートサーバーはjpサブドメイン@<fn>{subdomain}のネームサーバー@<fn>{jpnameservers}を返してきました。
つまりjpドメインのことはjpドメインの担当者に聞けということです。

//footnote[sakuradomain][.vs.sakura.ne.jpで終わるドメイン名]
//footnote[subdomain][.jpはTLD(Top Level Domain)なのでサブドメインと呼んでいいか微妙ですが……]
//footnote[jpnameservers][a.dns.jp, b.dns.jp, c.dns.jp, d.dns.jp, e.dns.jp, f.dns.jp, g.dns.jpの7つ]

=== 1月14日 午前9時52分 DNSクエリ送信(2回目)

jpドメインの担当者というのは要は@<kw>{JPRS, 日本レジストリサービス}のことです。
サーバーちゃんの住所を聞き出す手がかりを得たクライアントちゃんは、
夜が明けるのを待ってから今度はJPRSのDNSサーバーに対して手紙を書きました。

実を言うと、jpドメインのネームサーバーを管理しているのはJPRSその人ですが、
実際の運用は、JPRSを含めた5つの独立した組織によって行われています。
これはルートサーバーが世界に13個あるのと同じく、
ネームサーバーがダウンした際のリスクを可能な限り最小限に抑えるためです。

2回目のDNSレイヤーの内容は、トランザクションIDを除いて前回ルートサーバーに対して送ったのと全く同じ内容です。
DNSはなるべく高速に動作するように簡明に設計されているため、
このようにパケットの内容を容易に再利用できるようになっています。
決してクライアントちゃんがサボっているわけではありません。

=== 1月27日 午後2時30分 DNSレスポンス受信(2回目)

ふたたび待つこと2週間。すでに1月が終わりに近づいてきました。

クライアントちゃんの元にJPRSからの返信が届きました。サーバーちゃんの住所はまだわかりません。
今度はsakura.ne.jpの権威サーバーである、さくらインターネットのネームサーバー@<fn>{sakurans}の情報が返ってきました。
#@# 余裕があれば権威サーバーについてここに書く

ここに来てようやくさくらインターネットの影に辿り着きました。
サーバーちゃんの住所ゲットまであと一息です。

//footnote[sakurans][ns1.dns.ne.jp, ns2.dns.ne.jp]

=== 1月27日 午後6時56分 DNSクエリ送信(3回目)

同じやり取りも3回繰り返すと飽きてしまいますね。
けれどクライアントちゃんはサーバーちゃんに手紙を届けるために、めげずにDNSのパケットを送り続けます。
今度は先ほど入手したさくらインターネットのネームサーバーの住所に対して、
みたびサーバーちゃんの住所を尋ねる手紙を出します。

ところで、先ほどからクライアントちゃんは気軽にあちこちにDNSパケットを送ったり受け取ったりしていますが、
これはDNSが@<kw>{UDPプロトコル}の上で動作しているからです@<fn>{dns-over-tcp}。
UDPは、後ほど解説するTCPと異なり、相手のマシンとの接続を確立する必要がないため、
気軽に任意のアドレスにいきなりデータを送りつけることができます。

逆に言うと、TCPのような再送要求やエラー訂正などの機能を持たないため、
伝送経路のどこかでパケットが損失した場合、
クライアントちゃんは返事を待ちぼうけということになってしまいます。

もっとも、それもタイムアウトするまでですが。

//footnote[dns-over-tcp][DNS自体はTCP上でも動作することが義務付けられています。]

=== 2月10日 午前9時35分 DNSレスポンス受信(3回目)

2月に突入しました。
幸い3度目のクエリも待って待って待ちぼうけという事にはならず、
ポストに投函してから2週間経って再び返事が帰ってきました。
そこには待ち望んでいたサーバーちゃんの住所がくっきりと書かれています。

これにて名前解決完了。ようやくサーバーちゃんに対してパケットを送りつけることができるようになります。
もうあちこちのネームサーバーとパケットをやりとりする必要はありません。

年が明けてから41日間、実時間にして110ミリ秒が経過しました。
この調子で年内にサーバーちゃんとの通信を終えることができるのでしょうか。
クライアントちゃんの通信はまだまだこれからです。

== TCP接続ハンドシェイク

=== 2月19日 午後3時27分 TCPハンドシェイク SYNパケット送信

各地のネームサーバーの協力を得て、無事サーバーちゃんの住所を入手したクライアントちゃんですが、
お淑やかなクライアントちゃんはいきなり本題の手紙を送りつけるような真似はしません。
まずはサーバーちゃんにご挨拶をします。

TCP上の通信では、データ伝送を行う前にコネクションの確立という処理を行う必要があります。
これは、相手のサーバーが通信可能な状態であることを保証したり、
以降のデータが正しい順序で到着することを保証するための@<kw>{シーケンス番号}を互いに交換したりするためです。

シーケンス番号とは、現在のパケットが送信しているデータが、全体のデータのうちのどの部分に該当するのかを示す値であり、
ハンドシェイクで最初にランダムな値にセットされ、以降データを送信するごとに増加していく値です。
TCPは双方向通信なので、このシーケンス番号はサーバーとクライアントで別々の値を保持しています。

クライアントちゃんはランダムに生成したシーケンス番号を端に添えて、
サーバーちゃんに文通してよいかを問う内容の手紙を作りました。
色よい返事が返ってくることを期待して、再びポストに投函しました。
手紙はいよいよ石狩に向かいます。

ところで、サーバーちゃんの住所が判明してから最初にサーバーちゃんにコンタクトをとるまで9日もかかっています。
きっとバのつくイベントで忙しかったのでしょう。妬ましい。@<fn>{valentine}

//footnote[valentine][実際の理由はおそらくローカルホストのDNSサーバーからアプリケーション(curl)にDNS情報を受け渡す際にオーバーヘッドが発生するためです。]

=== 2月27日 午後6時30分 TCPハンドシェイク SYNパケット到達

北海道――新千歳空港から車で50分の石狩の大地に、目的のデータセンターは存在します。
冬の北海道の空気は冷たく厳しく、この時期の気温は昼間でも0度を上回ることはありません。
この冷涼な外気がサーバールームから効率的に排熱するのです。

すぐ脇を通る道は国道337号線です。地元ではかつて天売島に住んでいた鳥の名前からとって
「オロロンライン」と呼ばれるこの道は、眼前に手稲山を望むゆったりとした広い道路です。
小樽からこの道を進んで左手側、空港のターミナルを髣髴とさせる白い横長な建物が石狩データセンターです。

この場所でサーバーちゃんは他のサーバーと肩を並べて@<fn>{vps}、静かに443番ポートを開けて待ち続けています。
今回キャプチャしたパケットの中で最も数が多かったのはARPのパケットでした。
512台@<fn>{subnet-mask}近い数のサーバーがARPで常に囁きあい、互いの居場所を確認しあっている状態といえるかもしれません。

そんな退屈な生活の中で、サーバーちゃんはクライアントちゃんからの手紙を受け取りました。
クライアントちゃんが手紙を投函してから8日目のことです。伝送には22ミリ秒かかりました。
冒頭では東京と石狩の物理的な片道時間は5.9ミリ秒と述べましたが、当然これは理想的な通信のことであり、
実際には無線通信やルーティングなどにおけるオーバーヘッドによってそれ以上の時間がかかります。

受け取った手紙は一般的なTCPハンドシェイクでした。手紙にはシーケンス番号が添えられています。
クライアントちゃんからの久しぶりの手紙に喜んだサーバーちゃんは、喜んで通信を受け入れました。

//footnote[vps][もちろんVPSなので物理的なサーバーマシンとして存在しているわけではありませんが。]
//footnote[subnet-mask][今回使用したサーバーはサブネットマスク23ビットという中途半端な値のネットワークに繋がっていました。]

=== 2月27日 午後8時11分 TCPハンドシェイク SYN+ACKパケット送信

サーバーはTCPによる通信を受け入れた証として、クライアントからのSYNパケットに対する応答を返します。
サーバーはクライアントから受け取ったシーケンス番号を認識し、これに1を加えた値を返答パケットに記して送ります。
シーケンス番号は送信するデータの先頭バイトを表すので、本来はデータを送信しない段階では加算しないのですが、
ハンドシェイクにおいてはパケットを正しく受け取った印として特別に1を加算します。

同時に、サーバー側でもシーケンス番号を生成して返信用のパケットに記します。
これでサーバーとクライアントの間で一対のランダムなシーケンス番号が初期化されます。

サーバーちゃんはクライアントちゃんに向けて通信可能な旨を記した手紙を書きました。
このパケットはSYNとACKのフラグが立てられているため、@<kw>{SYN+ACKパケット}などと呼ばれます。

=== 3月9日 午前3時43分 TCPハンドシェイク SYN+ACKパケット到達

=== 3月9日 午前4時5分 TCPハンドシェイク ACKパケット送信

=== 3月16日 午後3時21分 TCPハンドシェイク ACKパケット到達

このように、TCPのハンドシェイクでは通信路を3回通る必要があるため、
@<kw>{3ウェイ・ハンドシェイク}とも呼ばれます。

== TLSハンドシェイク

TLSハンドシェイクが終わって、クライアントちゃんとサーバーちゃんは無事
通信が開始できるようになりました。
クライアントちゃんはさっそく本題の質問を投げかけようと思ったのですが、
ここでふと思ったことがあります。

クライアントちゃんとサーバーちゃんは、ハガキを使って互いにやり取りをしています。
ハガキには特に何も細工をしていないので、書いている内容は周りの人には丸見えです。
クライアントちゃんがポストに投函しに行くまではいいとしても、
そこから先、ルーターから先のことに関しては何も保証できません。
実はポストの中に盗撮カメラが仕掛けてあるかもしれませんし、
郵便局員がハガキの内容をチラ見するかもしれませんし、
サーバーちゃんの上司に内容を検閲されているかもしれません。

これは困ります。花も恥じらう乙女であるところのクライアントちゃんは、
サーバーちゃん以外の誰にも手紙の内容を知られたくありません。
クライアントちゃんはサーバーちゃんとのやり取りを暗号化するため、
@<kw>{TLS, Transport Layer Security}を使用することにしました。

TLSは、TCPのようなコネクションの上で暗号技術を使って通信の機密性や完全性を確保するための仕組みです。
公開鍵暗号を用いて安全に交換した鍵を使って共通鍵暗号を行い、
暗号化されたコネクションをアプリケーションに提供します。

この記事は暗号理論の説明はいたしません。ちょこちょこ出てくる用語の説明はほとんどしていません。
なので、やり取りする情報のさらなる意味を知りたい方は、
ぜひ他の専門書を参照してください。

=== 3月9日 午後7時27分 ClientHello 送信

TLSそのものは暗号を行う方式ではなく、
別に定められた暗号方式を使って通信を行うための仕組みです。
そのため、TLSハンドシェイクではこの暗号方式を決めることがコアになります。

暗号というものは時間が立つに連れて、その弱点が明らかになったり、
コンピューターの性能向上とともに解読に必要な時間が短くなるなどで弱くなります。
そのため、情報セキュリティにおいて暗号を利用する際には、
使用する暗号方式を新しい物に移行できることが重要になります。
TLSは将来開発しうる暗号方式を受け入れる事ができる、
一種のフレームワークとして作られています。

今回はクライアントちゃんとサーバーちゃんとで暗号通信を行いますが、
当然ながらお互いに使える暗号方式は限られており、
事前にそれらはわかりません。
TLSではこの暗号方式を後述するCipherSuiteと呼ばれる16bitの識別子で取り扱います。
ClientHelloではクライアントちゃんが使うことができる好みのCipherSuiteのリストを
サーバーちゃんに送ります。

他にもバージョン番号やセッション再開(resumption)やmaster secret生成に使用する乱数、使用できる署名及びアルゴリズムのリストなどが送信されます。

==== CipherSuite

CipherSuiteとは鍵交換アルゴリズム(Key Exchange)、暗号アルゴリズム(Cipher)、メッセージ認証符号(MAC)の組み合わせであり、
16bitの識別子が割り振られています。
例えば TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 は鍵交換にECDHE_RSA、
暗号にAES_256_GCM、メッセージ認証符号にSHA384を使用することを示しています。

==== master secret

ある意味ではハンドシェイクの成果物とも言える、
通信当事者以外には秘密でありながら、通信当事者同士で共有された値です。
長さは48バイトであり、同じく秘密に共有されたpremaster secretと、
Helloで交換した乱数から擬似乱数関数(PRF)を用いて生成されます。
擬似乱数関数はTLSではハッシュ関数の様な使い方をする関数であり、
入力から出力が一意に定まるが、出力から入力を予測することは不可能な関数です。

=== 3月19日 午前10時30分 ClientHello 受信

配達に10日もかかっています。おそらく津軽海峡を超えるのに時間がかかったのでしょう。

ClientHelloを受信したら、その中のCipherSuiteのリストや署名及びハッシュアルゴリズムのリストから実際に使用する方式を選定します。
この選定方法はアプリケーションによって異なり、管理者が自由に設定を行えます。

=== 3月20日 午後7時20分 ServerHello, Certificate, ServerKeyExchange 送信

サーバーちゃんはクライアントちゃんからClientHelloを受けて、返事をします。
この返事は複数のメッセージで構成されることがあります。

ServerHelloでは実際に使用するCipherSuiteとセッション再開やmaster secret生成に使用する乱数などの情報を送ります。

Certificateではサーバー証明書、及びその証明書を検証するために必要な証明書をクライアントちゃんに送ります。
この証明書は先に決定したCipherSuiteで使えるものでなければなりません。
運用上、実際には証明書の方を認証局から発行してもらい、
簡単にいじれるものではないため、証明書に適合したCipherSuiteを選ぶことになります。
このCertificateは任意ですが、これを送らないということは、
通信先であるサーバーが、本当に想定されるサーバーなのかの手がかりを与えない、匿名な通信となります。
暗号通信において、通信相手が攻撃者などではなく、意図した通信相手であることを保証することは重要です。

ServerKeyExchangeでは公開鍵や署名をクライアントちゃんに送ります。
具体的なデータ内容などはCipherSuiteで指定した鍵交換アルゴリズムによって異なります。
このServerKeyExchangeは任意であり、単純なRSAのように
Certificateに公開鍵や署名を含む場合にはこのメッセージは送られません。
しかしながら、この公開鍵が署名と一体であるということは、
すべての通信でこれを使いまわしているということであり、安全性に疑問が残ります。
将来的に公開鍵とセットである秘密鍵が破られた際に、後から秘匿性が失われる恐れがあります。
そのため、鍵を使い捨てにして前方秘匿性(forward secrecy)を確保することができる、
DHE等のアルゴリズムが推奨されています。
この使い捨ての鍵を証明書とは別に送信する際にServerKeyExchangeは使われます。

最後にServerHelloDoneを送ることで、返事を終わらせます。

=== 3月31日 午後10時26分 ServerHello, Certificate, ServerKeyExchange 受信

もうまもなく新年度が始まろうとするときに、サーバーちゃんから証明書などが届きます。

これらの情報を元に証明書の検証を行い、通信相手がサーバーちゃん自身であることを確認します。
そして、ここで始めてサーバーの公開鍵が手に入ります。
公開鍵暗号において、暗号化に必要なのは相手の公開鍵ですので、
裏を返せばここまでの通信内容はすべて暗号化されていない平文の状態で行われていたということです。
この暗号化されていないハンドシェイク部分は度々脅威にさらされてきました。
最近で言えばOpenSSLのCVE-2015-0204(FREAK)という脆弱性は、
このハンドシェイク時に中間者攻撃により脆弱な暗号化方式に切り替えさせられ、
通信の機密性を脅かすというものでした。

その後、サーバーちゃんに送るための公開鍵を用意します。
公開鍵を生成するにあたり、暗号論的な安全性を持った乱数を用いて秘密鍵を生成し、
指定されたCipherSuiteに従って公開鍵を生成し、次のメッセージに載せます。
そして、自身はその自分の秘密鍵と受け取った公開鍵からpremaster secretを求めます。(Diffie-Hello等)
これによりmaster secretをクライアント側で用意できるようになります。
master secretを用意したらpremaster secretはすぐに抹消します。
もしこの2人のお便りを盗み見してる人に、premaster secretが漏れてしまえば、
そこからmaster secretが計算できるので、あっという間に秘密が破られてしまうからです。

=== 4月1日 午前9時39分 ClientKeyExchange, ChangeCipherSpec, Finished 送信

翌朝、クライアントちゃんはサーバーちゃんとの秘密のやり取りに初めて取りかかります。

ClientKeyExchangeでは、自分が使用する公開鍵などを送ります。
RSAの場合にはpremaster secretを暗号化して送りますが、
通常良く使われるDiffie-Hellmanやその派生では、
その公開鍵を交換することでpremaster secretが計算可能です。

ChangeCipherSpecは次からハンドシェイクの結果に基づき、暗号化したデータを送るというメッセージであり、
いよいよ次から暗号化したデータに突入します。

Finishedは暗号化された最初のメッセージです。
と言っても、データ本体を暗号化するCipherSuiteで指定された、例えばAESのような暗号化ではなく、
きちんとハンドシェイクが成功したかを検証するためのverify_dataを送ります。
このverify_dataはmaster secretとこれまでのハンドシェイクメッセージから擬似乱数関数を通すことで生成され、
通信が改ざんされていなければサーバーちゃんはこれを検証することができます。

=== 4月9日 午前8時10分 ClientKeyExchange, ChangeCipherSpec, Finished 受信

クライアントちゃんの公開鍵とともに、初めての秘密のお便りが届きました。
もう、秘密のコネクションの確立は目前です！

サーバーちゃんは受け取った公開鍵と、自分が送った公開鍵に対応する秘密鍵からpremaster secretを求めます。
この値は鍵交換アルゴリズムの性質からクライアントちゃんと同じものになっているはずです。
もしそうでなければお便りは改ざんされています！
そして、そこからmaster secretを計算し、クライアントちゃんと同様にpremaster secretは抹消します。

これで準備は整いました！暗号化されたFinishedの中身をTLSで定められた手法でサーバーちゃんも試しに生成してみて、
送られてきたものと一致していることを確認します。
もし一致してなければその通信は危険な状態に晒されていることになります。
fatalレベルのアラートを送って通信を切断します。

もし正しかったら、鍵交換に成功です！

=== 4月9日 午後3時10分 NewSessionTicket, ChangeCipherSpec, Finished 送信

さて、サーバーちゃんもきちんと検証用の情報を送り、ハンドシェイクに成功したことをクライアントちゃんに伝えます。

NewSessionTicketでは、セッション再開に必要な、サーバーちゃんが持っているべき情報を、
クライアントちゃんに暗号化した状態で持ってもらうための情報です。
実はハンドシェイクの冒頭のHelloで、
お互いにTLS Session Resumption without Server-Side Stateを利用するかどうかの情報が交換できます。
もしお互いにこれを利用できるのであれば、セッション再開情報をサーバーちゃんが持たずに、
このチケットという形でクライアントちゃんに持ってもらい、
再開時にサーバーちゃんがそれを受け取り復号化して、セッションを再開できます。
が、今回は1回しか通信を行わないので、セッション再開の話はなしにしましょう。

ChangeCipherSpecとFinishedはクライアントちゃんと同じような流れです。ちょっと擬似乱数関数に入れる情報が違うだけです。

=== 4月19日 午前5時49分 NewSessionTicket, ChangeCipherSpec, Finished 受信

クライアントちゃんもハンドシェイクの成功を確認できる時が来ました！

セッション再開用のチケットを引き出しにしまいつつ、
送られてきたデータを検証します。
これで正常な通信の確立が確認できたらいよいよ暗号通信の始まりです。

== HTTPリクエストとレスポンス

#@# 暗号の内容を決めた二人は、ようやく本題に入る。

== TLS終了アラート

#@# 話を終えた二人は、暗号通信の最後のステップを行う。

== TCPコネクション切断

#@# 十分に話をした二人は一時の別れを告げる。(手紙で)

== 最後に

これは文章でした。
