/* ============================================================
   LOGIC POP — Script completo
   Arquitetura de 3 camadas independentes:
   1. Tela inicial  → removida do DOM ao jogar
   2. Cutscene      → criada/removida do DOM dinamicamente
   3. Jogo          → mostrado/escondido via display
   ============================================================ */

/* ----------------------------------------------------------
   SONS — Web Audio API
   ---------------------------------------------------------- */
const audioCtx = new (window.AudioContext || window.webkitAudioContext)();

function playCuteSound(tipo) {
  try {
    const ctx = audioCtx;
    const g = ctx.createGain(); g.connect(ctx.destination);
    const osc = ctx.createOscillator(); osc.connect(g);
    const now = ctx.currentTime;
    if (tipo === 'click') {
      osc.type='sine'; osc.frequency.setValueAtTime(880,now); osc.frequency.exponentialRampToValueAtTime(660,now+.08);
      g.gain.setValueAtTime(.18,now); g.gain.exponentialRampToValueAtTime(.001,now+.1);
      osc.start(now); osc.stop(now+.12);
    } else if (tipo === 'select') {
      osc.type='sine'; osc.frequency.setValueAtTime(523,now); osc.frequency.setValueAtTime(659,now+.06);
      g.gain.setValueAtTime(.15,now); g.gain.exponentialRampToValueAtTime(.001,now+.15);
      osc.start(now); osc.stop(now+.18);
    } else if (tipo === 'success') {
      [523,659,784,1047].forEach((f,i)=>{
        const o2=ctx.createOscillator(); const g2=ctx.createGain();
        o2.connect(g2); g2.connect(ctx.destination); o2.type='sine';
        o2.frequency.setValueAtTime(f,now+i*.08);
        g2.gain.setValueAtTime(.2,now+i*.08); g2.gain.exponentialRampToValueAtTime(.001,now+i*.08+.2);
        o2.start(now+i*.08); o2.stop(now+i*.08+.22);
      }); return;
    } else if (tipo === 'error') {
      osc.type='sine'; osc.frequency.setValueAtTime(330,now); osc.frequency.exponentialRampToValueAtTime(200,now+.22);
      g.gain.setValueAtTime(.14,now); g.gain.exponentialRampToValueAtTime(.001,now+.25);
      osc.start(now); osc.stop(now+.28);
    }
    osc.start && osc.start(now); osc.stop && osc.stop(now+.3);
  } catch(e) {}
}

/* ----------------------------------------------------------
   LAYER 1 — TELA INICIAL
   ---------------------------------------------------------- */
function telaInicioJogar() {
  playCuteSound('click');
  const el = document.getElementById('tela-inicio');
  if (el) el.remove();
  // primeira vez: mostra cutscene antes do jogo
  csAbrir(function() { _iniciarJogoReal(); });
}

function telaInicioRever() {
  playCuteSound('click');
  const el = document.getElementById('tela-inicio');
  if (el) el.remove();
  csAbrir(function() { _iniciarJogoReal(); });
}

/* ----------------------------------------------------------
   LAYER 2 — CUTSCENE
   Criada dinamicamente, removida do DOM ao fechar.
   csAbrir(callback) — callback é chamado ao finalizar
   ---------------------------------------------------------- */
const CS_CENAS = [
  { img:'cena1.jpg', txt:'No Reino dos Doces, existe um castelo protegido por cinco portões encantados.' },
  { img:'cena2.jpg', txt:'Cada portão só se abre com uma PopKey — uma chave mágica em formato de pirulito! 🍭' },
  { img:'cena3.jpg', txt:'Blu e Pinky foram escolhidos para disputar quem será o novo guardião do reino! 🐻' },
  { img:'cena4.jpg', txt:'A Carta Verde revela um enigma de lógica a cada rodada. 💚' },
  { img:'cena5.jpg', txt:'Encontre a expressão equivalente antes do seu rival! Seja rápido! ⚡' },
  { img:'cena6.jpg', txt:'Conquiste pelo menos 3 PopKeys para vencer e se tornar o guardião do Reino dos Doces! 🏆' },
];

let _csIdx = 0;
let _csCallback = null;

function csAbrir(callback) {
  _csIdx = 0;
  _csCallback = callback || null;

  // injeta keyframes se necessário
  if (!document.getElementById('cs-keyframes')) {
    const st = document.createElement('style');
    st.id = 'cs-keyframes';
    st.textContent = '@keyframes csKB{0%{transform:scale(1) translate(0,0)}50%{transform:scale(1.04) translate(-.5%,.3%)}100%{transform:scale(1.02) translate(.5%,-.3%)}}';
    document.head.appendChild(st);
  }

  // cria o elemento cutscene
  const el = document.createElement('div');
  el.id = 'cutscene';
  el.style.cssText = 'position:fixed;inset:0;width:100vw;height:100vh;overflow:hidden;background:#2b0b4f;z-index:9999;';

  el.innerHTML = `
    <img id="cs-img" style="position:absolute;inset:0;width:100%;height:100%;object-fit:cover;object-position:center;animation:csKB 12s ease-in-out infinite alternate;" src="" alt=""/>
    <div style="position:absolute;inset:0;pointer-events:none;z-index:1;background:linear-gradient(to bottom,transparent 45%,rgba(10,2,30,.6) 100%);"></div>
    <div id="cs-fade" style="position:absolute;inset:0;background:#000;opacity:0;pointer-events:none;z-index:8;transition:opacity .35s ease;"></div>
    <div id="cs-badge" style="position:absolute;top:18px;left:18px;z-index:5;width:56px;height:56px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-family:'Silkscreen',monospace;font-size:1.4rem;color:#3a1060;background:linear-gradient(180deg,#ffb8e0,#ff80c0);border:4px solid rgba(255,255,255,.9);box-shadow:0 4px 0 #c04080;">1</div>
    <div id="cs-prog" style="position:absolute;top:18px;right:18px;z-index:5;font-family:'Silkscreen',monospace;font-size:.7rem;color:#fff;background:rgba(40,10,100,.78);border:3px solid rgba(255,255,255,.4);border-radius:999px;padding:5px 16px;letter-spacing:.08em;">1 / 6</div>
    <div id="cs-textbox" style="position:absolute;left:50%;bottom:90px;transform:translateX(-50%);width:min(82vw,1050px);background:#fff4fb;border:4px solid #f47bb7;border-radius:28px;padding:20px 32px;z-index:3;text-align:center;box-shadow:0 6px 0 #b03070;">
      <p id="cs-txt" style="font-family:'Nunito',sans-serif;font-weight:900;font-size:clamp(.95rem,2.1vw,1.35rem);color:#3a1060;line-height:1.45;margin:0;"></p>
    </div>
    <div style="position:absolute;left:50%;bottom:22px;transform:translateX(-50%);z-index:4;display:flex;gap:12px;flex-wrap:wrap;justify-content:center;">
      <button id="cs-back" onclick="csBack()" style="font-family:'Silkscreen',monospace;font-size:.7rem;letter-spacing:.06em;background:linear-gradient(180deg,#70c0ff,#3080e0);color:#fff;border:3px solid rgba(255,255,255,.88);border-radius:14px;padding:11px 22px;cursor:pointer;box-shadow:0 5px 0 #1858b0;">← VOLTAR</button>
      <button id="cs-next" onclick="csNext()" style="font-family:'Silkscreen',monospace;font-size:.7rem;letter-spacing:.06em;background:linear-gradient(180deg,#ffd060,#e09020);color:#3a1060;border:3px solid rgba(255,255,255,.88);border-radius:14px;padding:11px 22px;cursor:pointer;box-shadow:0 5px 0 #a06010;">PRÓXIMO →</button>
      <button onclick="csPular()" style="font-family:'Silkscreen',monospace;font-size:.7rem;letter-spacing:.06em;background:linear-gradient(180deg,#c080ff,#8040d0);color:#fff;border:3px solid rgba(255,255,255,.88);border-radius:14px;padding:11px 22px;cursor:pointer;box-shadow:0 5px 0 #5020a0;">PULAR ▶▶</button>
      <button id="cs-start" onclick="csFechar()" style="display:none;font-family:'Silkscreen',monospace;font-size:.7rem;letter-spacing:.06em;background:linear-gradient(180deg,#60e090,#28a850);color:#fff;border:3px solid rgba(255,255,255,.88);border-radius:14px;padding:11px 22px;cursor:pointer;box-shadow:0 5px 0 #107830;">COMEÇAR JOGO ⭐</button>
    </div>
  `;

  document.body.appendChild(el);
  csRender();
}

function csFechar() {
  const el = document.getElementById('cutscene');
  if (el) el.remove();
  if (typeof _csCallback === 'function') {
    const cb = _csCallback; _csCallback = null; cb();
  }
}

function csPular() { csFechar(); }

function csNext() {
  if (_csIdx < CS_CENAS.length - 1) { _csIdx++; csRender(); }
}

function csBack() {
  if (_csIdx > 0) { _csIdx--; csRender(); }
}

function csRender() {
  const cena = CS_CENAS[_csIdx];
  const total = CS_CENAS.length;
  const fade = document.getElementById('cs-fade');
  if (fade) fade.style.opacity = '1';
  setTimeout(() => {
    const img = document.getElementById('cs-img');
    if (img) { img.style.animation='none'; img.src=cena.img; img.offsetHeight; img.style.animation='csKB 12s ease-in-out infinite alternate'; }
    const txt = document.getElementById('cs-txt'); if (txt) txt.textContent = cena.txt;
    const badge = document.getElementById('cs-badge'); if (badge) badge.textContent = _csIdx+1;
    const prog = document.getElementById('cs-prog'); if (prog) prog.textContent = (_csIdx+1)+' / '+total;
    const back = document.getElementById('cs-back');
    if (back) { back.disabled=(_csIdx===0); back.style.opacity=_csIdx===0?'.38':'1'; }
    const next = document.getElementById('cs-next');
    const start = document.getElementById('cs-start');
    if (_csIdx === total-1) { if(next) next.style.display='none'; if(start) start.style.display='inline-flex'; }
    else { if(next) next.style.display=''; if(start) start.style.display='none'; }
    if (fade) fade.style.opacity = '0';
  }, 320);
}

/* ----------------------------------------------------------
   LAYER 3 — JOGO
   ---------------------------------------------------------- */
const RODADAS_BASE = [
  { central:'P → Q',     correta:'¬P ∨ Q',  opcoes:['¬P ∨ Q','P ∧ Q','Q → P','¬P ∧ Q','P ∨ Q'],        explicacao:'P → Q  é equivalente a  ¬P ∨ Q',    regra:'Implicação' },
  { central:'¬(P ∧ Q)',  correta:'¬P ∨ ¬Q', opcoes:['¬P ∨ ¬Q','P ∨ Q','¬P ∧ ¬Q','P ∧ Q','P → Q'],      explicacao:'¬(P ∧ Q)  é equivalente a  ¬P ∨ ¬Q', regra:'De Morgan ∧' },
  { central:'P ∨ Q',     correta:'Q ∨ P',   opcoes:['Q ∨ P','P ∧ Q','¬P ∨ Q','¬(P ∨ Q)','P → Q'],      explicacao:'P ∨ Q  é equivalente a  Q ∨ P',      regra:'Comutatividade ∨' },
  { central:'P ∧ Q',     correta:'Q ∧ P',   opcoes:['Q ∧ P','P ∨ Q','P → Q','¬P ∧ Q','¬Q ∨ P'],        explicacao:'P ∧ Q  é equivalente a  Q ∧ P',      regra:'Comutatividade ∧' },
  { central:'¬(P ∨ Q)',  correta:'¬P ∧ ¬Q', opcoes:['¬P ∧ ¬Q','¬P ∨ ¬Q','P ∧ Q','Q ∨ P','P → Q'],     explicacao:'¬(P ∨ Q)  é equivalente a  ¬P ∧ ¬Q', regra:'De Morgan ∨' },
];

let rodadas=[], rodadaAtual=0;
let pontosJ1=0, pontosJ2=0, acertosJ1=0, acertosJ2=0, errosJ1=0, errosJ2=0;
let tempoTotalJ1=0, tempoTotalJ2=0;
let tempoRodada=60, intervaloTimer=null;
let jogoAtivo=false, aguardandoProxima=false;

const POSICOES_J = [
  { x: 50, y: 30 }, // W / ↑
  { x: 20, y: 62 }, // A / ←
  { x: 50, y: 62 }, // S / ↓
  { x: 80, y: 62 }  // D / →
];

function embaralhar4Opcoes(opcoes, correta) {
  const opcoesUnicas = [...new Set(opcoes)];
  const erradas = opcoesUnicas.filter(op => op !== correta);

  for (let i = erradas.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [erradas[i], erradas[j]] = [erradas[j], erradas[i]];
  }

  const escolhidas = [correta, ...erradas.slice(0, 3)];

  for (let i = escolhidas.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [escolhidas[i], escolhidas[j]] = [escolhidas[j], escolhidas[i]];
  }

  return {
    arr: escolhidas,
    idxCorreta: escolhidas.indexOf(correta)
  };
}

function iniciarJogo() { _iniciarJogoReal(); }

function _iniciarJogoReal() {
  const ti = document.getElementById('tela-inicio'); if(ti) ti.remove();

  rodadas = RODADAS_BASE.map(r => {
    const r1 = embaralhar4Opcoes(r.opcoes, r.correta);
    const r2 = embaralhar4Opcoes(r.opcoes, r.correta);

    return {
      ...r,
      opcoesJ1: r1.arr,
      corretaJ1: r1.idxCorreta,
      opcoesJ2: r2.arr,
      corretaJ2: r2.idxCorreta
    };
  });
  rodadaAtual=0; pontosJ1=pontosJ2=0; acertosJ1=acertosJ2=errosJ1=errosJ2=0; tempoTotalJ1=tempoTotalJ2=0;
  jogoAtivo=false; aguardandoProxima=false;

  const tj = document.getElementById('tela-jogo'); if(tj) { tj.style.display='flex'; tj.classList.add('ativa'); }
  const tf = document.getElementById('tela-final'); if(tf) { tf.style.display='none'; tf.classList.remove('ativa'); }

  atualizarPlacar();
  iniciarRodada();
}

function iniciarRodada() {
  if (rodadaAtual >= rodadas.length) {
    finalizarJogo();
    return;
  }

  const r = rodadas[rodadaAtual];

  aguardandoProxima = false;
  jogoAtivo = true;

  document.getElementById('num-rodada').textContent = rodadaAtual + 1;
  document.getElementById('total-rodadas').textContent = rodadas.length;

  const fp = document.getElementById('feedback-painel');
  fp.classList.remove('acerto', 'erro');

  document.getElementById('feedback-icone').textContent = '';
  document.getElementById('feedback-msg').textContent = 'ENCONTRE O EQUIVALENTE!';
  document.getElementById('feedback-exp').textContent = '';

  ['carta-j1', 'carta-central', 'carta-j2'].forEach(id => {
    document.getElementById(id).classList.remove('flash-acerto', 'flash-erro');
  });

  renderizarCentral(r.central);
  renderizarCartaJogador('formulas-j1', r.opcoesJ1, 'j1');
  renderizarCartaJogador('formulas-j2', r.opcoesJ2, 'j2');

  tempoRodada = 60;
  atualizarCronometro();

  clearInterval(intervaloTimer);
  intervaloTimer = setInterval(tickTimer, 1000);
}

function renderizarCentral(prop) {
  const c=document.getElementById('formulas-central'); c.innerHTML='';
  const w=document.createElement('div'); w.className='central-proposicao';
  const f=document.createElement('div'); f.className='central-formula-texto'; f.textContent=prop;
  w.appendChild(f); c.appendChild(w);
}

function renderizarCartaJogador(containerId, opcoes, tipo) {
  const container = document.getElementById(containerId);
  container.innerHTML = '';

  opcoes.slice(0, 4).forEach((formula, i) => {
    const pos = POSICOES_J[i];

    const b = document.createElement('div');
    b.classList.add('formula-bolinha', `bolinha-${tipo}`);
    b.style.left = pos.x + '%';
    b.style.top = pos.y + '%';
    b.dataset.idx = i;

    const t = document.createElement('span');
    t.classList.add('formula-texto');
    t.textContent = formula;

    b.appendChild(t);
    container.appendChild(b);
  });
}

document.addEventListener('keydown', function(e) {
  if (!jogoAtivo || aguardandoProxima) return;

  const teclasJ1 = {
    w: 0,
    W: 0,
    a: 1,
    A: 1,
    s: 2,
    S: 2,
    d: 3,
    D: 3
  };

  const teclasJ2 = {
    ArrowUp: 0,
    ArrowLeft: 1,
    ArrowDown: 2,
    ArrowRight: 3
  };

  if (Object.prototype.hasOwnProperty.call(teclasJ1, e.key)) {
    e.preventDefault();
    playCuteSound('select');
    verificarResposta(1, teclasJ1[e.key]);
    return;
  }

  if (Object.prototype.hasOwnProperty.call(teclasJ2, e.key)) {
    e.preventDefault();
    playCuteSound('select');
    verificarResposta(2, teclasJ2[e.key]);
    return;
  }
});

function verificarResposta(jogador, escolha) {
  const r = rodadas[rodadaAtual];

  const correta = jogador === 1 ? r.corretaJ1 : r.corretaJ2;
  const cartaId = jogador === 1 ? 'carta-j1' : 'carta-j2';

  if (escolha === correta) {
    acertou(jogador, r, cartaId);
  } else {
    errou(jogador, cartaId);
  }
}

function acertou(jogador,r,cartaId) {
  clearInterval(intervaloTimer); jogoAtivo=false; aguardandoProxima=true;
  const tempoGasto=60-tempoRodada;
  if(jogador===1){pontosJ1++;acertosJ1++;tempoTotalJ1+=tempoGasto;}
  else{pontosJ2++;acertosJ2++;tempoTotalJ2+=tempoGasto;}
  playCuteSound('success'); atualizarPlacar();
  mostrarPopPontos(jogador,'+1');
  document.getElementById(cartaId).classList.add('flash-acerto');
  document.getElementById('carta-central').classList.add('flash-acerto');
  criarSparkleBurst(cartaId,8); criarSparkleBurst('carta-central',5);
  mostrarFeedback(true,jogador===1?'✅ Blu acertou! 🐻':'✅ Pinky acertou! 🐻',r.explicacao);
  setTimeout(()=>{ rodadaAtual++; iniciarRodada(); },3000);
}

function errou(jogador,cartaId) {
  if(jogador===1) errosJ1++; else errosJ2++;
  playCuteSound('error');
  document.getElementById(cartaId).classList.add('flash-erro');
  setTimeout(()=>document.getElementById(cartaId).classList.remove('flash-erro'),400);
  mostrarFeedback(false,'❌ Tente outra!','');
  setTimeout(()=>{
    if(!aguardandoProxima){
      const fp=document.getElementById('feedback-painel'); fp.classList.remove('acerto','erro');
      document.getElementById('feedback-icone').textContent='';
      document.getElementById('feedback-msg').textContent='ENCONTRE O EQUIVALENTE!';
      document.getElementById('feedback-exp').textContent='';
    }
  },1200);
}

function tickTimer() {
  tempoRodada--; atualizarCronometro();
  if(tempoRodada<=0){
    clearInterval(intervaloTimer); jogoAtivo=false; aguardandoProxima=true;
    mostrarFeedback(false,'⏰ Tempo esgotado!','Ninguém pontuou nesta rodada.');
    setTimeout(()=>{ rodadaAtual++; iniciarRodada(); },2500);
  }
}

function atualizarCronometro() {
  const min=Math.floor(tempoRodada/60); const seg=tempoRodada%60;
  document.getElementById('tempo-texto').textContent=`${min}:${seg.toString().padStart(2,'0')}`;
  const cron=document.getElementById('cronometro');
  if(tempoRodada<=10) cron.classList.add('urgente'); else cron.classList.remove('urgente');
}

function atualizarPlacar() {
  document.getElementById('pts-j1').textContent=pontosJ1;
  document.getElementById('pts-j2').textContent=pontosJ2;
}

function finalizarJogo() {
  clearInterval(intervaloTimer); jogoAtivo=false;
  document.getElementById('final-pts-j1').textContent=pontosJ1;
  document.getElementById('final-pts-j2').textContent=pontosJ2;
  document.getElementById('final-ac-j1').textContent=acertosJ1;
  document.getElementById('final-ac-j2').textContent=acertosJ2;
  document.getElementById('final-err-j1').textContent=errosJ1;
  document.getElementById('final-err-j2').textContent=errosJ2;
  document.getElementById('final-cat-j1').textContent='⏱ '+tempoTotalJ1+'s de resposta';
  document.getElementById('final-cat-j2').textContent='⏱ '+tempoTotalJ2+'s de resposta';
  let vencedor;
  if(pontosJ1>pontosJ2) vencedor='🏆 Blu venceu! 🐻';
  else if(pontosJ2>pontosJ1) vencedor='🏆 Pinky venceu! 🐻';
  else if(tempoTotalJ1<tempoTotalJ2) vencedor='🏆 Empate! Blu venceu pelo tempo! 🐻';
  else if(tempoTotalJ2<tempoTotalJ1) vencedor='🏆 Empate! Pinky venceu pelo tempo! 🐻';
  else vencedor='🤝 Empate perfeito! Blu e Pinky são lógicos! 🐻';
  document.getElementById('final-vencedor').textContent=vencedor;
  document.getElementById('final-msg-logica').textContent='A lógica venceu! 🎉';

  const tj=document.getElementById('tela-jogo'); if(tj){tj.style.display='none';tj.classList.remove('ativa');}
  const tf=document.getElementById('tela-final'); if(tf){tf.style.display='flex';tf.classList.add('ativa');}
}

function mostrarFeedback(acerto,msg,exp) {
  const fp=document.getElementById('feedback-painel');
  const ico=document.getElementById('feedback-icone');
  const m=document.getElementById('feedback-msg');
  const e=document.getElementById('feedback-exp');
  fp.classList.remove('acerto','erro'); fp.classList.add(acerto?'acerto':'erro');
  ico.textContent=acerto?'✓':'✗'; m.textContent=msg; e.textContent=exp||'';
}

function mostrarPopPontos(jogador,texto) {
  const pop=document.createElement('div');
  pop.classList.add('score-pop',texto.startsWith('+')?'positivo':'negativo');
  pop.textContent=texto;
  const elem=document.getElementById(jogador===1?'placar-j1':'placar-j2');
  const rect=elem.getBoundingClientRect();
  pop.style.left=(rect.left+rect.width/2)+'px'; pop.style.top=(rect.top-10)+'px';
  document.body.appendChild(pop); setTimeout(()=>pop.remove(),1300);
}

function criarSparkleBurst(elementId,count=6) {
  const elem=document.getElementById(elementId); if(!elem) return;
  const rect=elem.getBoundingClientRect();
  const cx=rect.left+rect.width/2; const cy=rect.top+rect.height/2;
  const radius=Math.min(rect.width,rect.height)*.42;
  const chars=['✦','✧','★','♡'];
  for(let i=0;i<count;i++){
    const spark=document.createElement('div'); spark.className='sparkle-burst';
    spark.textContent=chars[i%chars.length];
    const angle=(Math.PI*2*i)/count; const offset=radius*(.6+Math.random()*.5);
    spark.style.left=(cx+Math.cos(angle)*offset)+'px'; spark.style.top=(cy+Math.sin(angle)*offset)+'px';
    spark.style.animationDelay=(i*20)+'ms'; document.body.appendChild(spark);
    setTimeout(()=>spark.remove(),800);
  }
}

function reiniciarJogo() {
  const tf=document.getElementById('tela-final'); if(tf){tf.style.display='none';tf.classList.remove('ativa');}
  _iniciarJogoReal(); // pula cutscene no reinício
}

function toggleGuia() {
  document.getElementById('modal-guia').classList.toggle('oculto');
}

