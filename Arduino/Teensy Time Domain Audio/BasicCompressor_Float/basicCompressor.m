
fs_Hz = 44100;      %sample rate
attack_sec = 0.01;  %doesn't end up being exact.  Error is dependent upon release time, too.
release_sec = 0.1;  %doesn't end up being exact.  Error is dependent upon attack time, too.
thresh_dBFS = -10;  %threshold for compression to start
comp_ratio = 5;     %compression ratio.  "5" means 5:1

%generate test signal
t_sec = ([1:4*fs_Hz]-1)/fs_Hz;
freq_Hz = 2000;  %frequency of my test signal
wav = sqrt(2)*sin(2*pi*freq_Hz*t_sec);  %rms of 1.0 (aka 0 dB)

%make step change in amplitude
flag_isAttackTest = 0;  %do an attack or a release test?
if (flag_isAttackTest)
    levels_dBFS = [-30 0];%two amplitude levels
else
    levels_dBFS = [0 -30];  %two amplitude levels
end
t_bound_sec = 1;        %time of transition
I=find(t_sec < t_bound_sec);
wav(I) = sqrt(10.^(0.1*levels_dBFS(1)))*wav(I);
I=find(t_sec >= t_bound_sec);
wav(I) = sqrt(10.^(0.1*levels_dBFS(2)))*wav(I);

%% process the signal

%get signal power
wav_pow = wav.^2;

%smooth via attack and release
attack_const = exp(-1/(attack_sec*fs_Hz));
release_const = exp(-1/(release_sec*fs_Hz));
new_wav_pow = ones(size(wav_pow)); new_wav_pow(1) = wav_pow(1);
for I=2:length(wav_pow)
    if (wav_pow(I) > wav_pow(I-1))
        %attack
        new_wav_pow(I) = new_wav_pow(I-1)*attack_const + wav_pow(I)*(1-attack_const);
    else
        %release
        new_wav_pow(I) = new_wav_pow(I-1)*release_const + wav_pow(I)*(1-release_const);
    end
end

%get power relative to threshold
thresh_pow_FS = 10.^(0.1*thresh_dBFS);
%wav_pow_rel_thresh = wav_pow ./ thresh_pow_FS;
wav_pow_rel_thresh = new_wav_pow ./ thresh_pow_FS;

%compute gain
gain_pow = ones(size(wav_pow_rel_thresh));
I=find(wav_pow_rel_thresh > 1.0);
gain_pow(I) = 10.^((1/comp_ratio - 1)*log10(wav_pow_rel_thresh(I)));

    

%% plots
figure;try;setFigureTallerWide;catch;end
ax=[];

subplot(3,2,1);
plot(t_sec,wav);
xlabel('Time (sec)');
ylabel('Raw Waveform');
ylim([-1 1]);
hold on; 
plot(xlim,sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
plot(xlim,-sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(3,2,2);
plot(t_sec,10*log10(wav_pow));
xlabel('Time (sec)');
ylabel('Signal Power (dBFS)');
ylim([-40 10]);
hold on; 
plot(xlim,thresh_dBFS*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(3,2,3);
plot(t_sec,10*log10(new_wav_pow));
xlabel('Time (sec)');
ylabel('Smoothed Signal Power (dBFS)');
ylim([-40 10]);
hold on; 
plot(xlim,thresh_dBFS*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;


subplot(3,2,4);
plot(t_sec,10*log10(wav_pow_rel_thresh));
xlabel('Time (sec)');
ylabel('Signal Power (dB) Rel Threshold');
ylim([-20 20]);
hold on; 
plot(xlim,[0 0],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(3,2,5);
plot(t_sec,10*log10(gain_pow));
xlabel('Time (sec)');
ylabel('Target Gain (dB)');
ylim([-12 0]);
hold on; 
plot(xlim,[0 0],'k--','linewidth',2);
hold off
ax(end+1)=gca;

linkaxes(ax,'x');
xlim([0 2]);

% subplot(3,2,5);
% plot(t_sec,10*log10(new_gain_pow));
% xlabel('Time (sec)');
% ylabel('Smoothed Gain (dB)');
% ylim([-10 0]);
% hold on; 
% plot(xlim,[0 0],'k--','linewidth',2);
% hold off

subplot(3,2,6);
dt_sec = t_sec-t_bound_sec;
gain_rel_final_dB = 10*log10(gain_pow/gain_pow(end));
semilogx(dt_sec,gain_rel_final_dB)
xlabel('Time (sec) Since Transition');
ylabel('Gain Relative to Final Gain (dB)');
xlim([1e-4 1]);
if (flag_isAttackTest)
    ylim([-1 10]);
    time_const_val = 2;
    hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
    I=find(gain_rel_final_dB > time_const_val);I=I(end)+1;

    hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
    yl=ylim;
    text(dt_sec(I),yl(2)-0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
        'horizontalAlignment','center','verticalAlignment','top',...
        'backgroundcolor','white');

else
    ylim([-10 1]);
    time_const_val = -2;
    hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
    I=find(gain_rel_final_dB < time_const_val);I=I(end)+1;
 
    hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
    yl=ylim;
    text(dt_sec(I),yl(1)+0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
        'horizontalAlignment','center','verticalAlignment','bottom',...
        'backgroundcolor','white');

end
   



