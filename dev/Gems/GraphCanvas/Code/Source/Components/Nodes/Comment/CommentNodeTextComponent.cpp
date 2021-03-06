/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include <functional>

#include <QGraphicsItem>
#include <QGraphicsGridLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QPainter>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Comment/CommentNodeTextComponent.h>

#include <Components/Nodes/Comment/CommentTextGraphicsWidget.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <Utils/ConversionUtils.h>

namespace GraphCanvas
{
    /////////////////////////////////////
    // CommentNodeTextComponentSaveData
    /////////////////////////////////////

    CommentNodeTextComponent::CommentNodeTextComponentSaveData::CommentNodeTextComponentSaveData()
        : m_callback(nullptr)
    {
    }

    CommentNodeTextComponent::CommentNodeTextComponentSaveData::CommentNodeTextComponentSaveData(CommentNodeTextComponent* nodeComponent)
        : m_callback(nodeComponent)
    {
    }

    void CommentNodeTextComponent::CommentNodeTextComponentSaveData::operator=(const CommentNodeTextComponentSaveData& other)
    {
        // Purposefully skipping over the callback.
        m_comment = other.m_comment;
        m_fontConfiguration = other.m_fontConfiguration;
    }

    void CommentNodeTextComponent::CommentNodeTextComponentSaveData::OnCommentChanged()
    {
        if (m_callback)
        {
            m_callback->OnCommentChanged();
            SignalDirty();
        }
    }

    void CommentNodeTextComponent::CommentNodeTextComponentSaveData::UpdateStyleOverrides()
    {
        if (m_callback)
        {
            m_callback->UpdateStyleOverrides();
            SignalDirty();
        }
    }

    AZStd::string CommentNodeTextComponent::CommentNodeTextComponentSaveData::GetLabel() const
    {
        if (m_callback)
        {
            switch (m_callback->GetCommentMode())
            {
                case CommentMode::BlockComment:
                {
                    return "Group Name";
                }
                case CommentMode::Comment:
                {
                    return "Comment";
                }
                default:
                {
                    break;
                }
            }
        }

        return "Title";
    }

    /////////////////////////////
    // CommentNodeTextComponent
    /////////////////////////////
    bool CommentNodeTextComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 2)
        {
            AZ::Crc32 commentId = AZ_CRC("Comment", 0x9474526c);
            AZ::Crc32 fontId = AZ_CRC("FontSettings", 0x9d90b4cf);

            CommentNodeTextComponent::CommentNodeTextComponentSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(commentId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_comment);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(fontId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_fontConfiguration);
            }

            classElement.RemoveElementByName(commentId);
            classElement.RemoveElementByName(fontId);

            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    void CommentNodeTextComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CommentNodeTextComponentSaveData>()
                ->Version(1)
                ->Field("Comment", &CommentNodeTextComponentSaveData::m_comment)
                ->Field("FontSettings", &CommentNodeTextComponentSaveData::m_fontConfiguration)
                ;

            serializeContext->Class<CommentNodeTextComponent, GraphCanvasPropertyComponent>()
                ->Version(3, &CommentNodeTextComponentVersionConverter)
                ->Field("SaveData", &CommentNodeTextComponent::m_saveData)
            ;

            serializeContext->Class<FontConfiguration>()
                ->Field("FontColor", &FontConfiguration::m_fontColor)
                ->Field("FontFamily", &FontConfiguration::m_fontFamily)
                ->Field("PixelSize", &FontConfiguration::m_pixelSize)
                ->Field("Weight", &FontConfiguration::m_weight)
                ->Field("Style", &FontConfiguration::m_style)
                ->Field("VAlign", &FontConfiguration::m_verticalAlignment)
                ->Field("HAlign", &FontConfiguration::m_horizontalAlignment)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class < CommentNodeTextComponentSaveData >("SaveData", "The save information regarding a comment node")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextComponentSaveData::m_comment, "Title", "The comment to display on this node")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextComponentSaveData::OnCommentChanged)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CommentNodeTextComponentSaveData::GetLabel)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextComponentSaveData::m_fontConfiguration, "Font Settings", "The font settings used to render the font in the comment.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextComponentSaveData::UpdateStyleOverrides)
                ;

                editContext->Class<CommentNodeTextComponent>("Comment", "The node's customizable properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextComponent::m_saveData, "SaveData", "The modifiable information about this comment.")
                ;

                editContext->Class<FontConfiguration>("Font Settings", "Various settings used to control a font.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_fontColor, "Font Color", "The color that the font of this comment should render with")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_fontFamily, "Font Family", "The font family to use when rendering this comment.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_pixelSize, "Pixel Size", "The size of the font(in pixels)")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 200)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_weight, "Weight", "The weight of the font")
                        ->EnumAttribute(QFont::Thin, "Thin")
                        ->EnumAttribute(QFont::ExtraLight, "Extra Light")
                        ->EnumAttribute(QFont::Light, "Light")
                        ->EnumAttribute(QFont::Normal, "Normal")
                        ->EnumAttribute(QFont::Medium, "Medium")
                        ->EnumAttribute(QFont::DemiBold, "Demi-Bold")
                        ->EnumAttribute(QFont::Bold, "Bold")
                        ->EnumAttribute(QFont::ExtraBold, "Extra Bold")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_style, "Style", "The style of the font")
                        ->EnumAttribute(QFont::Style::StyleNormal, "Normal")
                        ->EnumAttribute(QFont::Style::StyleItalic, "Italic")
                        ->EnumAttribute(QFont::Style::StyleOblique, "Oblique")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_verticalAlignment, "Vertical Alignment", "The Vertical Alignment of the font")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignTop, "Top")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignVCenter, "Middle")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignBottom, "Bottom")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_horizontalAlignment, "Horizontal Alignment", "The Horizontal Alignment of the font")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignLeft, "Left")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignHCenter, "Center")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignRight, "Right")
                    ;
            }
        }
    }

    CommentNodeTextComponent::CommentNodeTextComponent()
        : m_saveData(this)
        , m_commentTextWidget(nullptr)
        , m_commentMode(CommentMode::Comment)
    {
    }

    CommentNodeTextComponent::CommentNodeTextComponent(AZStd::string_view initialText)
        : CommentNodeTextComponent()
    {
        m_saveData.m_comment = initialText;
    }

    void CommentNodeTextComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();
        
        m_saveData.m_fontConfiguration.InitializePixelSize();
        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CommentNodeTextComponent::Activate()
    {
        if (m_commentTextWidget == nullptr)
        {
            m_commentTextWidget = aznew CommentTextGraphicsWidget(GetEntityId());
            m_commentTextWidget->SetStyle(Styling::Elements::CommentText);
        }

        GraphCanvasPropertyComponent::Activate();

        CommentRequestBus::Handler::BusConnect(GetEntityId());
        CommentLayoutRequestBus::Handler::BusConnect(GetEntityId());

        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        
        m_commentTextWidget->Activate();
    }

    void CommentNodeTextComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        NodeNotificationBus::Handler::BusDisconnect();

        CommentLayoutRequestBus::Handler::BusDisconnect();
        CommentRequestBus::Handler::BusDisconnect();

        m_commentTextWidget->Deactivate();
    }

    void CommentNodeTextComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);

        if (m_commentTextWidget->GetCommentMode() == CommentMode::Comment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, false);
        }
        else if (m_commentTextWidget->GetCommentMode() == CommentMode::BlockComment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);
        }

        AZ::EntityId grid;
        SceneRequestBus::EventResult(grid, sceneId, &SceneRequests::GetGrid);

        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetGrid, grid);        

        m_commentTextWidget->OnAddedToScene();
        UpdateStyleOverrides();
        OnCommentChanged();

        m_saveData.RegisterIds(GetEntityId(), sceneId);
    }

    void CommentNodeTextComponent::SetComment(const AZStd::string& comment)
    {
        if (m_saveData.m_comment.compare(comment) != 0)
        {
            m_saveData.m_comment = comment;
            m_commentTextWidget->SetComment(comment);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
    }

    void CommentNodeTextComponent::SetCommentMode(CommentMode commentMode)
    {
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);
        m_commentTextWidget->SetCommentMode(commentMode);
        m_commentMode = commentMode;

        if (m_commentTextWidget->GetCommentMode() == CommentMode::Comment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, false);
        }
        else if (m_commentTextWidget->GetCommentMode() == CommentMode::BlockComment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);
        }
    }

    CommentMode CommentNodeTextComponent::GetCommentMode() const
    {
        return m_commentMode;
    }

    const AZStd::string& CommentNodeTextComponent::GetComment() const
    {
        return m_saveData.m_comment;
    }

    QGraphicsLayoutItem* CommentNodeTextComponent::GetGraphicsLayoutItem()
    {
        return m_commentTextWidget;
    }

    void CommentNodeTextComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        CommentNodeTextComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<CommentNodeTextComponentSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void CommentNodeTextComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        CommentNodeTextComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<CommentNodeTextComponentSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    void CommentNodeTextComponent::OnCommentChanged()
    {
        if (m_commentTextWidget)
        {
            CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentChanged, m_saveData.m_comment);

            m_commentTextWidget->SetComment(m_saveData.m_comment);
        }
    }

    void CommentNodeTextComponent::UpdateStyleOverrides()
    {
        CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentFontReloadBegin);

        QColor fontColor = ConversionUtils::AZToQColor(m_saveData.m_fontConfiguration.m_fontColor);

        Styling::StyleHelper& styleHelper = m_commentTextWidget->GetStyleHelper();

        styleHelper.AddAttributeOverride(Styling::Attribute::Color, fontColor);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontFamily, QString(m_saveData.m_fontConfiguration.m_fontFamily.c_str()));
        styleHelper.AddAttributeOverride(Styling::Attribute::FontSize, m_saveData.m_fontConfiguration.m_pixelSize);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontWeight, m_saveData.m_fontConfiguration.m_weight);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontStyle, m_saveData.m_fontConfiguration.m_style);
        styleHelper.AddAttributeOverride(Styling::Attribute::TextAlignment, m_saveData.m_fontConfiguration.m_horizontalAlignment);
        styleHelper.AddAttributeOverride(Styling::Attribute::TextVerticalAlignment, m_saveData.m_fontConfiguration.m_verticalAlignment);

        m_commentTextWidget->OnStyleChanged();

        CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentFontReloadEnd);
    }
}